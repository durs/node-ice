#ifndef _node_utils_h__
#define _node_utils_h__

#include <iostream>
#include <node.h>
#include <v8.h>
#include <vector>
#include <boost/shared_ptr.hpp>

//-------------------------------------------------------------------------------------

extern v8::Persistent<v8::String> g_v8s_length;
extern v8::Persistent<v8::String> g_v8s_type;
extern v8::Persistent<v8::String> g_v8s_subtype;
extern v8::Persistent<v8::String> g_v8s_optional;
extern v8::Persistent<v8::String> g_v8s_write;
extern v8::Persistent<v8::String> g_v8s_read;

//-------------------------------------------------------------------------------------

//#define NODE_DEBUG_(strm, msg) strm << msg << std::endl;

#define NODE_DEBUG_(strm, msg) { std::string s("### "); s+=(msg); s+="\r\n"; strm.write(s.c_str(), s.size()); }
#define NODE_DEBUG_MSG(msg) NODE_DEBUG_(std::cout, msg)
#define NODE_DEBUG_ERR(msg) NODE_DEBUG_(std::cerr, msg)

#define EXCEPTION_FORMAT(err, prefix, e) { err=std::string(prefix); err+="Exception: ["; err+=e.what(); err+="]"; std::replace( err.begin(), err.end(), '\n', ' '); }

#define NODE_ERROR(msg) v8::ThrowException(v8::Exception::Error(v8::String::New(msg)))

#define NODE_DEFINE_CONSTRUCTOR(type, callback) \
	static inline v8::Handle<v8::Value> node_##callback(const v8::Arguments &args) { \
		v8::HandleScope scope; \
		type *p = new type(); \
		if (!p) return scope.Close(NODE_ERROR("Invalid node instance")); \
		p->Wrap(args.Holder()); \
		p->callback(args); \
		return scope.Close(args.This()); } \
	void callback(const v8::Arguments &args);

#define NODE_DEFINE_METHOD(type, callback) \
	static inline v8::Handle<v8::Value> node_##callback(const v8::Arguments &args) { \
		v8::HandleScope scope; \
		type *p = ObjectWrap::Unwrap<type>(args.Holder()); \
		if (!p) return scope.Close(NODE_ERROR("Invalid node instance")); \
		return scope.Close(p->callback(args)); } \
	v8::Handle<v8::Value> callback(const v8::Arguments &args);

#define NODE_DEFINE_PROPERTY(type, callback) \
	static inline v8::Handle<v8::Value> node_##callback(v8::Local<v8::String> prop, const v8::AccessorInfo& info) { \
		v8::HandleScope scope; \
		type *me = ObjectWrap::Unwrap<type>(info.This()); \
		if (!me) return v8::Undefined(); \
		return scope.Close(me->callback(info.This())); } \
	v8::Handle<v8::Value> callback(v8::Local<v8::Object> &me);

#define NODE_ASSIGN_METHOD(t, name, callback) \
	NODE_SET_PROTOTYPE_METHOD(t, name, node_##callback);

#define NODE_ASSIGN_PROPERTY(t, name, callback) \
	t->PrototypeTemplate()->SetAccessor(v8::String::New(name), Node_##callback);

//-------------------------------------------------------------------------------------

inline std::string str2str(std::wstring &src)
{
	return std::string(src.begin(), src.end());
}

inline std::wstring str2str(std::string &src)
{
	return std::wstring(src.begin(), src.end());
}

//-------------------------------------------------------------------------------------
inline bool is_empty(const v8::Handle<v8::Value> &v)
{
	return v.IsEmpty() || v->IsUndefined() || v->IsNull();
}

inline bool node2dbl(v8::Handle<v8::Number> &num, double &value)
{
	if (num.IsEmpty()) return false;
	value = num->Value();
	return true;
}

inline bool node2dbl(v8::Handle<v8::Value> &val, double &value)
{
	if (is_empty(val)) return false;
	v8::Local<v8::Number> &num = val->ToNumber();
	if (num.IsEmpty()) return false;
	return node2dbl(num, value);
}

inline bool node2int(v8::Handle<v8::Integer> &num, __int64 &value)
{
	if (num.IsEmpty()) return false;
	value = num->Value();
	return true;
}

inline bool node2int(v8::Handle<v8::Value> &val, __int64 &value)
{
	if (is_empty(val)) return false;
	v8::Local<v8::Integer> &num = val->ToInteger();
	if (num.IsEmpty()) return false;
	return node2int(num, value);
}

inline bool node2int(v8::Handle<v8::Int32> &num, int &value)
{
	if (num.IsEmpty()) return false;
	value = num->Value();
	return true;
}

inline bool node2int(v8::Handle<v8::Value> &val, int &value)
{
	if (is_empty(val)) return false;
	v8::Local<v8::Int32> &num = val->ToInt32();
	if (num.IsEmpty()) return false;
	return node2int(num, value);
}

inline bool node2str(const v8::String::Utf8Value &str, std::string &value)
{
	int len = str.length();
	value.resize(len);
	memcpy(&value[0], *str, len);
	return true;
}

inline bool node2str(const v8::Handle<v8::String> &val, std::string &value)
{
	if (is_empty(val)) return false;
	return node2str(v8::String::Utf8Value(val), value);
}

inline bool node2str(const v8::Handle<v8::Value> &val, std::string &value)
{
	if (is_empty(val)) return false;
	return node2str(v8::String::Utf8Value(val), value);
}

inline bool node2str(const v8::Handle<v8::String> &str, std::wstring &value)
{
	if (str.IsEmpty()) return false;
	int len = str->Length();
	value.resize(len);
	str->Write((uint16_t*)value.c_str(), 0, len);
	return true;
}

inline bool node2str(const v8::Handle<v8::Value> &val, std::wstring &value)
{
	if (is_empty(val)) return false;
	v8::Local<v8::String> &str = val->ToString();
	if (str.IsEmpty()) return false;
	return node2str(str, value);
}

inline bool node2func(v8::Handle<v8::Value> &val, v8::Local<v8::Function> &value)
{
	v8::HandleScope scope;
	if (val.IsEmpty() || !val->IsFunction()) return false;
	value = scope.Close(val.As<v8::Function>());
	return !value.IsEmpty();
}

inline bool node2obj(v8::Handle<v8::Value> &val, v8::Local<v8::Object> &value)
{
	v8::HandleScope scope;
	if (val.IsEmpty() || !val->IsObject()) return false;
	value = scope.Close(val.As<v8::Object>());
	return !value.IsEmpty();
}

inline bool args2str(const v8::Arguments &args, int index, std::wstring &value)
{
	//if (index >= args.Length()) return false;
	return node2str(args[index], value);
}

inline bool args2str(const v8::Arguments &args, int index, std::string &value)
{
	//if (index >= args.Length()) return false;
	return node2str(args[index], value);
}

inline bool args2obj(const v8::Arguments &args, int index, v8::Local<v8::Object> &value)
{
	//if (index >= args.Length()) return false;
	return node2obj(args[index], value);
}

inline bool args2func(const v8::Arguments &args, int index, v8::Local<v8::Function> &value)
{
	//if (index >= args.Length()) return false;
	return node2func(args[index], value);
}

template<typename F>
inline bool objenum_t(v8::Handle<v8::Object> &obj, F func)
{
	bool rcode = true;
	v8::HandleScope scope;
	v8::Local<v8::Array> &keys = obj->GetOwnPropertyNames();
	uint32_t keycnt = keys.IsEmpty() ? 0 : keys->Length();
	for (uint32_t keyno = 0; keyno < keycnt; keyno ++)
	{
		v8::Local<v8::Value> &key = keys->Get(keyno);
		v8::Local<v8::Value> &val = obj->Get(key);
		rcode = func(key, val);
		if (!rcode) break;
	}
	return rcode;
}

template<typename F>
inline bool objenum(v8::Handle<v8::Object> &obj, F func)
{
	return objenum_t(obj, [func](v8::Local<v8::Value> &key, v8::Local<v8::Value> &val)->bool{
		std::string name;
		node2str(key, name);
		return func(name, val);
	});
}

template<typename F>
inline bool objenum_w(v8::Handle<v8::Object> &obj, F func)
{
	return objenum_t(obj, [func](v8::Local<v8::Value> &key, v8::Local<v8::Value> &val)->bool{
		std::wstring name;
		node2str(key, name);
		return func(name, val);
	});
}

class objenumerator
{
public:
	inline objenumerator(v8::Local<v8::Object> &o): index(0), obj(o) { init(); }
	inline objenumerator(v8::Local<v8::Value> &v): index(0) { node2obj(v, obj); init(); }

	inline bool empty()
	{
		return length == 0 && keycnt == 0;
	}

	inline v8::Local<v8::Value> next()
	{
		v8::HandleScope scope;
		if (index < length) return scope.Close(obj->Get(index++));
		if (index < keycnt) return scope.Close(obj->Get(keys->Get(index++)));
		index ++;
		return scope.Close(v8::Undefined());
	}

	inline v8::Local<v8::Value> next(const v8::Handle<v8::Value> &key)
	{
		v8::HandleScope scope;
		if (length != 0) return scope.Close(next());
		if (!key.IsEmpty() && keycnt > 0) return scope.Close(obj->Get(key));
		return scope.Close(v8::Undefined());		
	}

	inline v8::Local<v8::Value> next(const std::wstring &key)
	{
		v8::HandleScope scope;
		if (length != 0) return scope.Close(next());
		if (!key.empty() && keycnt > 0) return scope.Close(next(v8::String::New((uint16_t*)key.c_str(), key.length())));
		return scope.Close(v8::Undefined());		
	}

private:
	v8::Local<v8::Object> obj;
	v8::Local<v8::Array> keys;
	uint32_t keycnt, length, index;

	inline void init()
	{
		v8::HandleScope scope;
		length = keycnt = 0;
		if (obj.IsEmpty()) return;
		v8::Local<v8::Value> &vlen = obj->Get(g_v8s_length);
		if (!vlen.IsEmpty()) length = vlen->Int32Value();
		if (length == 0) 
		{
			keys = obj->GetOwnPropertyNames();
			if (!keys.IsEmpty()) keycnt = keys->Length();
		}
	}
};


template<typename T>
inline T *node2_t(v8::Handle<v8::Object> &obj)
{
	node::ObjectWrap *pobj = (obj->InternalFieldCount() > 0) ? static_cast<node::ObjectWrap*>(obj->GetPointerFromInternalField(0)) : 0;
	return (pobj != 0) ? dynamic_cast<T*>(pobj) : 0;
}

template<typename T>
inline T *node2_t(v8::Handle<v8::Value> &val)
{
	v8::Local<v8::Object> obj;
	return node2obj(val, obj) ? node2_t<T>(obj) : 0;
}

//-------------------------------------------------------------------------------------

class NodeCallback
{
public:
	v8::Persistent<v8::Object> me;
	v8::Persistent<v8::Function> func;

	inline NodeCallback() {}
	inline NodeCallback(v8::Handle<v8::Function> &cb): /*me(v8::Persistent<v8::Object>::New(v8::Null())),*/ func(v8::Persistent<v8::Function>::New(cb)) {}
	inline NodeCallback(v8::Handle<v8::Object> &obj, v8::Handle<v8::Function> &cb): me(v8::Persistent<v8::Object>::New(obj)), func(v8::Persistent<v8::Function>::New(cb)) {}
	inline ~NodeCallback() {}

	inline void Invoke(int argc, v8::Handle<v8::Value> argv[])
	{
		v8::HandleScope scope;
		v8::TryCatch try_catch;
		if (!func.IsEmpty()) func->Call(v8::Handle<v8::Object>(me), argc, argv);
		if (try_catch.HasCaught())
		{
			v8::Local<v8::Message> &err = try_catch.Message();
			if (!err.IsEmpty())
			{
				std::string errmsg;
				node2str(err->Get(), errmsg);
				NODE_DEBUG_MSG(std::string("Node Callback Exception: ") += errmsg);				
			}
		}
	}
};

class NodeAsync
{
public:
	boost::shared_ptr<NodeCallback> ptr;
	inline NodeAsync() {}
	inline NodeAsync(boost::shared_ptr<NodeCallback> &cb): ptr(cb) {}
	virtual ~NodeAsync() {	
		if (ptr) ptr.reset();
	}

	static inline void Call(boost::shared_ptr<NodeCallback> &cb)
	{
		NodeAsync *async = new NodeAsync(cb);
		async->Invoke();
	};

	inline void Invoke()
	{
		memset(&req, 0, sizeof(req));
		req.data = this;
		uv_async_init(uv_default_loop(), &req, do_process);
		uv_async_send(&req);
	}
	virtual void Process() 
	{ 
		if (ptr) ptr->Invoke(0, 0); 
	}

private:
	uv_async_t req;
	static void do_release(uv_handle_t* handle) 
	{
		NodeAsync *data = (NodeAsync *)handle->data;
		delete data;
	}
	static void do_process(uv_async_t *req, int status) 
	{
		NodeAsync *data = (NodeAsync *)req->data;
		data->Process();
		uv_close((uv_handle_t*)req, do_release);
	}
};

//-------------------------------------------------------------------------------------
#endif