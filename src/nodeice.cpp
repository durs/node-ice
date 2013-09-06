
#include "nodeice.h"

const Ice::Byte NodeIceSimpleT<Ice::Byte>::defval = 0;
const Ice::Short NodeIceSimpleT<Ice::Short>::defval = 0;
const Ice::Int NodeIceSimpleT<Ice::Int>::defval = 0;
const Ice::Long NodeIceSimpleT<Ice::Long>::defval = 0;
const Ice::Double NodeIceSimpleT<Ice::Double>::defval = 0;
const Ice::Float NodeIceSimpleT<Ice::Float>::defval = 0;
const std::wstring NodeIceSimpleT<std::wstring>::defval;
const bool NodeIceSimpleT<bool>::defval = false;

//-------------------------------------------------------------------------------------
// Common Ice utilities

static v8::Local<v8::Value> ice2node(Ice::InputStreamPtr &in, v8::Handle<v8::Object> &info)
{
	v8::HandleScope scope;
	in->startEncapsulation();
	in->endEncapsulation();
	scope.Close(v8::Undefined());
}

static v8::Local<v8::Value> ice2node(Ice::InputStreamPtr &s, v8::Handle<v8::Value> &info, bool root = true)
{
	v8::HandleScope scope;
	v8::Handle<v8::Value> val;
	std::wstring str;
	if (node2str(info, str))
	{
		s->startEncapsulation();
		if (str==L"string") ICE2STR(val, s)
		else if (str==L"byte") ICE2BYTE(val, s)
		else if (str==L"short") ICE2SHORT(val, s)
		else if (str==L"int") ICE2INT(val, s)
		else if (str==L"long") ICE2LONG(val, s)
		else if (str==L"bool") ICE2BOOL(val, s)
		else if (str==L"double") ICE2DOUBLE(val, s)
		else if (str==L"float") ICE2FLOAT(val, s)
		//else error
		s->endEncapsulation();
	}
	else
	{
		v8::Handle<v8::Object> obj;
		//if (node2obj(info, obj))  ice2node(in, obj);
	}
	if (val.IsEmpty()) val = v8::Undefined();
	return scope.Close(val);
}

static bool node2ice(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &info, v8::Local<v8::Value> &val, bool root = true);

static bool node2ice(Ice::OutputStreamPtr &s, v8::Handle<v8::Object> &info, v8::Local<v8::Value> &val, bool root)
{
	v8::HandleScope scope;
	v8::Local<v8::Object> obj;
	uint32_t cnt = 0;
	node2obj(val, obj);

	s->startEncapsulation();
	bool rcode = objenum_t(info, [&s, &cnt, obj](v8::Local<v8::Value> &key, v8::Local<v8::Value> &type)->bool{

		//std::wstring skey;
		//node2str(key, skey);

		v8::Local<v8::Value> val;
		if (!obj.IsEmpty()) 
		{
			// Try read by index (for array or arguments object)
			val = obj->Get(cnt);

			// Try read by key (for other object)
			if (val->IsUndefined()) 
				val = obj->Get(key);
		}

		//std::wstring sval;
		//node2str(val, sval);

		cnt ++;
		return node2ice(s, type, val, false);
	});
	s->endEncapsulation();
	return rcode;
}

static bool node2ice(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &info, v8::Local<v8::Value> &val, bool root)
{
	v8::HandleScope scope;
	std::wstring type, subtype;
	int optional = 0;

	// Try convert info to object
	v8::Local<v8::Object> obj;
	if (node2obj(info, obj))
	{
		// Try call write function
		/*
		v8::Local<v8::Value> &fwrite = obj->Get(g_v8s_write);
		if (!fwrite.IsEmpty() && fwrite->IsFunction())
		{
			v8::Local<v8::Function> &f = fwrite.As<v8::Function>();
			f->Call(val, 1, &val);
			return;
		}
		*/

		// Read type property and process as struct when not found
		v8::Local<v8::Value> &vtype = obj->Get(g_v8s_type);
		if (vtype.IsEmpty() || vtype->IsUndefined())
			return node2ice(s, obj, val, root);

		// Read other properties
		node2int(obj->Get(g_v8s_optional), optional);

		// Try convert type to object and process as struct on success
		v8::Local<v8::Object> obj2;
		if (node2obj(vtype, obj2))
			return node2ice(s, obj2, val, root);

		// Try convert type to string
		if (!node2str(vtype, type)) 
			return false;
	}

	// Try convert info to string
	else if (!node2str(info, type)) 
	{
		return false;
	}

	// Process as simple type
	bool result = true;
	if (root) s->startEncapsulation();
	if (type==L"string") ICE3STR(val, s)
	else if (type==L"byte") ICE3BYTE(val, s)
	else if (type==L"short") ICE3SHORT(val, s)
	else if (type==L"int") ICE3INT(val, s)
	else if (type==L"long") ICE3LONG(val, s)
	else if (type==L"bool") ICE3BOOL(val, s)
	else if (type==L"double") ICE3DOUBLE(val, s)
	else if (type==L"float") ICE3FLOAT(val, s)
	else if (type==L"array")
	{
		if (!obj.IsEmpty()) node2str(obj->Get(g_v8s_subtype), subtype);
		::std::vector<::Ice::Byte> buf;
		//buf.push_back(1);
		//buf.push_back(1);
		s->write(buf);
		//buf.push_back(1);
	}
	else result = false;
	if (root) s->endEncapsulation();
	return result;
}

//-------------------------------------------------------------------------------------
// Ice invocation result (for Node asynchronically process)

class IceResult: public NodeAsync
{
public:
	std::string err;
	Ice::InputStreamPtr strm;
	NodeIceTypePtr type;
	v8::Persistent<v8::Value> info;
	v8::Persistent<v8::Object> holder;
	v8::Persistent<v8::Value> value;
	inline IceResult() {}
	virtual ~IceResult() {}
	virtual void Process();
	inline bool IsVoid() { return !type && info.IsEmpty(); }
};

void IceResult::Process() 
{
	v8::HandleScope scope;

	// Parse output stream
	v8::Local<v8::Value> val;
	if (err.empty()) 
	try 
	{ 
		if (type) 
		{
			strm->startEncapsulation();
			type->read(strm, val, 0);
			strm->endEncapsulation();
		}
		else 
		{
			val = ice2node(strm, info, true); 
		}
	}
	catch (const Ice::UserException &e) { EXCEPTION_FORMAT(err, "Ice User ", e); }
	catch (const Ice::LocalException &e) { EXCEPTION_FORMAT(err, "Ice Local ", e); }
	catch (const std::exception &e) { EXCEPTION_FORMAT(err, "", e); }
	catch (...) { err = "Unknown Exception"; }
	if (val.IsEmpty()) val = v8::Local<v8::Value>::New(v8::Undefined()); 
	value = v8::Persistent<v8::Value>::New(val);
	
	// Debug message
	if (err.empty()) NODE_DEBUG_MSG("Ice.Callback finished")
	else NODE_DEBUG_ERR(std::string("Ice.Callback error: ") += err)

	// Callback invoke
	if (ptr)
	{
		v8::Handle<v8::Value> args[2];
		if (err.empty()) args[0] = v8::Undefined();
		else args[0] = v8::String::New(err.c_str());
		args[1] = val;
		ptr->Invoke(2, args); 
	}
}

//-------------------------------------------------------------------------------------
// Ice invocation callback (for Ice asynchronically process)

class IceCallback: public IceUtil::Shared
{
public:
	IceResult *result;
	inline IceCallback(IceResult *p): result(p) {}
	inline ~IceCallback() { if (result) delete result; }
	void process(const Ice::AsyncResultPtr &r) { std::string err; v8::Handle<v8::Value> val; do_process(r, true); }
	void do_process(const Ice::AsyncResultPtr& r, bool async);
};
typedef IceUtil::Handle<IceCallback> IceCallbackPtr;

// May be called in separate thread (Ice async callback)
void IceCallback::do_process(const Ice::AsyncResultPtr &r, bool async)
{
    Ice::ObjectPrx &prx = r->getProxy();
    try 
	{
		Ice::InputStreamPtr strm;
		std::vector<Ice::Byte> outParams;
		bool rcode = prx->end_ice_invoke(outParams, r);
		if (!rcode || !result->IsVoid())
		{
			Ice::CommunicatorPtr &ic = r->getCommunicator();
			if (!ic && !result->holder.IsEmpty())
			{
				NodeIceProxy *proxy = node::ObjectWrap::Unwrap<NodeIceProxy>(result->holder);
				if (proxy) ic = proxy->ic;
			}
			if (ic) strm = Ice::createInputStream(ic, outParams);
		}
		if (!strm) ;
		else if (rcode)
		{
			result->strm = strm;
		}
		else try
		{
			strm->throwException();
			//Ice::ReadObjectCallbackPtr cb(new Ice::ReadObjectCallback());
			//strm->readObject(cb);
		}
		catch(const Ice::UnmarshalOutOfBoundsException &)
		{
			result->err = "Remote User Exception (for detailed need Ice v3.5)";
		}
	} 
	catch (const Ice::UserException &e) { EXCEPTION_FORMAT(result->err, "Ice User ", e); }
	catch (const Ice::LocalException &e) { EXCEPTION_FORMAT(result->err, "Ice Local ", e); }
	catch (const std::exception &e) { EXCEPTION_FORMAT(result->err, "", e); }
	catch (...) { result->err = "Unknown Exception"; }

	// Process now
	if (!async) result->Process();

	// Start Node asynchronically invocation
	else 
	{
		result->Invoke();
		result = 0;
	}
}


//-------------------------------------------------------------------------------------
// Node registration

v8::Persistent<v8::ObjectTemplate> g_communicator;
v8::Persistent<v8::ObjectTemplate> g_proxy;

/*
v8::Handle<v8::Value> ice_initialize(const v8::Arguments& args)
{
	v8::HandleScope scope;
	v8::Handle<v8::Object> self = args.This();
	v8::Handle<v8::Value> retval = self->Get(v8::String::New("Communicator"));

	if(!retval->IsFunction()) return scope.Close(NODE_ERROR("Node Ice: Communicator object not found on Ice object"));
	v8::Handle<v8::Function> communicator_f = v8::Handle<v8::Function>::Cast(retval);

	v8::Handle<v8::Value>* _args = new v8::Handle<v8::Value>[args.Length()];
	for(int i = 0; i < args.Length();i++){
		_args[i] = args[i];
	}

	v8::Handle<v8::Object> communicator_o = communicator_f->NewInstance(args.Length(),_args);
	return scope.Close(communicator_o);
}
*/

void ice_node_register(v8::Handle<v8::Object> &target)
{
	v8::HandleScope scope;

	// Register ice methods
	//NODE_SET_METHOD(target, "initialize", ice_initialize);

	// Register ice simple types
	NodeIceTypeT<NodeIceSimpleT<Ice::Byte> >::node_register(target, "Byte", true);
	NodeIceTypeT<NodeIceSimpleT<Ice::Int> >::node_register(target, "Int", true);
	NodeIceTypeT<NodeIceSimpleT<Ice::Long> >::node_register(target, "Long", true);
	NodeIceTypeT<NodeIceSimpleT<Ice::Double> >::node_register(target, "Double", true);
	NodeIceTypeT<NodeIceSimpleT<Ice::Float> >::node_register(target, "Float", true);
	NodeIceTypeT<NodeIceSimpleT<std::wstring> >::node_register(target, "String", true);
	NodeIceTypeT<NodeIceSimpleT<bool> >::node_register(target, "Bool", true);

	// Register ice other types
	NodeIceTypeT<NodeIceSequence>::node_register(target, "Sequence", false);
	NodeIceTypeT<NodeIceDictionary>::node_register(target, "Dictionary", false);	
	NodeIceTypeT<NodeIceParam>::node_register(target, "Field", false);
	NodeIceTypeT<NodeIceParam>::node_register(target, "Argument", false);
	NodeIceTypeT<NodeIceMethod>::node_register(target, "Method", false);
	NodeIceTypeT<NodeIceStruct>::node_register(target, "Struct", false);
	//NodeIceTypeT<NodeIceClass>::node_register(target, "Class", false);
	//NodeIceTypeT<NodeIceInterface>::node_register(target, "Interface", false);
	//NodeIceTypeT<NodeIceException>::node_register(target, "Exception", false);

	// Register ice Communicator
	v8::Local<v8::FunctionTemplate> f_communicator = v8::FunctionTemplate::New(NodeIceCommunicator::node_init);
	g_communicator = v8::Persistent<v8::ObjectTemplate>::New(f_communicator->InstanceTemplate());
	g_communicator->SetInternalFieldCount(1);
	g_communicator->Set("__ice", target);
	f_communicator->SetClassName(v8::String::NewSymbol("Communicator"));
	NODE_SET_PROTOTYPE_METHOD(f_communicator, "done", NodeIceCommunicator::node_done);
	NODE_SET_PROTOTYPE_METHOD(f_communicator, "stringToProxy", NodeIceCommunicator::node_stringToProxy);
	NODE_SET_PROTOTYPE_METHOD(f_communicator, "propertyToProxy", NodeIceCommunicator::node_propertyToProxy);
	target->Set(v8::String::NewSymbol("Communicator"), f_communicator->GetFunction());

	// Register ice Proxy
	v8::Local<v8::FunctionTemplate> f_proxy = v8::FunctionTemplate::New(NodeIceProxy::node_init);
	g_proxy = v8::Persistent<v8::ObjectTemplate>::New(f_proxy->InstanceTemplate());
	g_proxy->SetInternalFieldCount(1);
	g_proxy->Set("__ice", target);
	f_proxy->SetClassName(v8::String::NewSymbol("Proxy"));
	NODE_SET_PROTOTYPE_METHOD(f_proxy, "invoke", NodeIceProxy::node_invoke);
	//NODE_ASSIGN_METHOD(t, "valueOf", value_of);
	//NODE_ASSIGN_METHOD(t, "toString", to_string);
	target->Set(v8::String::NewSymbol("Proxy"), f_proxy->GetFunction());

	NODE_DEBUG_MSG("NodeIce registered");
}

//-------------------------------------------------------------------------------------------------------
// NodeIceTypeT

template<typename T>
void NodeIceTypeT<T>::node_register(v8::Handle<v8::Object> &target, const char *name, bool as_instance)
{
	v8::HandleScope scope;
	v8::Local<v8::String> &id = v8::String::NewSymbol(name);
	v8::Local<v8::FunctionTemplate> &t_func = v8::FunctionTemplate::New(NodeIceTypeT::node_init);
	v8::Local<v8::ObjectTemplate> &t_obj = t_func->InstanceTemplate();
	t_obj->SetInternalFieldCount(1);
	t_obj->Set("__ice", target);
	t_func->SetClassName(id);

	//NODE_SET_PROTOTYPE_METHOD(f_communicator, "done", NodeIceCommunicator::node_done);
	//NODE_SET_PROTOTYPE_METHOD(f_communicator, "stringToProxy", NodeIceCommunicator::node_stringToProxy);
	//NODE_SET_PROTOTYPE_METHOD(f_communicator, "propertyToProxy", NodeIceCommunicator::node_propertyToProxy);

	if (as_instance) target->Set(id, t_obj->NewInstance());
	else target->Set(id, t_func->GetFunction());
}

template<typename T>
void NodeIceTypeT<T>::init(const v8::Arguments& args)
{
	type.reset(new T);
	type->init(args);
}

void NodeIceTypeBase::init(const v8::Arguments& args)
{
	v8::HandleScope scope;
	v8::Local<v8::Object> info;
	if (args2obj(args, 0, info)) objenum_t(info, [this](const v8::Local<v8::Value> &key, v8::Local<v8::Value> &val)->bool{
		this->set(key, val);
		return true;
	});
	else for(int i = 0; i < args.Length(); i ++)
	{
		set(v8::Int32::New(i), args[i]);
	}
}

NodeIceTypePtr NodeIceTypeBase::create(v8::Handle<v8::Value> &info)
{
	v8::HandleScope scope;
	NodeIceTypeWrap *wrap = node2_t<NodeIceTypeWrap>(info);
	if (wrap) return wrap->type;

	v8::Local<v8::Object> obj;
	if (node2obj(info, obj)) return NodeIceTypePtr(new NodeIceStruct(obj));

	std::wstring str;
	node2str(info, str);
	if (str==L"string") return NodeIceTypePtr(new NodeIceSimpleT<std::wstring>());
	if (str==L"byte") return NodeIceTypePtr(new NodeIceSimpleT<Ice::Byte>());
	if (str==L"short") return NodeIceTypePtr(new NodeIceSimpleT<Ice::Short>());
	if (str==L"int") return NodeIceTypePtr(new NodeIceSimpleT<Ice::Int>());
	if (str==L"long") return NodeIceTypePtr(new NodeIceSimpleT<Ice::Long>());
	if (str==L"bool") return NodeIceTypePtr(new NodeIceSimpleT<bool>());
	if (str==L"double") return NodeIceTypePtr(new NodeIceSimpleT<Ice::Double>());
	if (str==L"float") return NodeIceTypePtr(new NodeIceSimpleT<Ice::Float>());
	return NodeIceTypePtr();
}

template<typename T>
bool NodeIceSimpleT<T>::write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt) 
{ 
	if (opt != 0 && is_empty(p)) return true;
	T value = node2ice(p, defval);
	if (opt == 0) s->write(value); 
	else s->write(opt, IceUtil::Optional<T>(value));
	return true; 
}

template<typename T>
bool NodeIceSimpleT<T>::read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt) 
{ 
	assert(opt == 0);
	T value; 
	s->read(value); 
	p = ice2node(value);
	return true; 
}

void NodeIceSequence::init(const v8::Arguments& args)
{
	v8::HandleScope scope;
	int argcnt = args.Length();
	int argindex = -1;

	// Process type
	if (++argindex < argcnt)
		type = NodeIceTypeBase::create(args[argindex]);
}

bool NodeIceSequence::write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt)
{
	if (!type) return false;
	//assert(opt == 0);
	int cnt;
	bool rcode = true;
	v8::HandleScope scope;
	v8::Local<v8::Object> obj;
	if (!node2obj(p, obj))
	{
		s->writeSize((Ice::Int)0);
	}
	else if (node2int(obj->Get(g_v8s_length), cnt))
	{
		s->writeSize((Ice::Int)cnt);
		for (int i = 0; i < cnt; i ++)
		{
			v8::Local<v8::Value> &pi = obj->Get(i);
			if (!type->write(s, pi, 0))
			{
				rcode = false;
				break;
			}
		}	
	}
	else
	{
		s->writeSize((Ice::Int)0);
		/*
		v8::Local<v8::Array> &keys = obj->GetOwnPropertyNames();
		uint32_t keycnt = keys.IsEmpty() ? 0 : keys->Length();
		s->writeSize((Ice::Int)keycnt);
		for (uint32_t keyno = 0; keyno < keycnt; keyno ++)
		{
			v8::Local<v8::Value> &key = keys->Get(keyno);
			v8::Local<v8::Value> &val = obj->Get(key);
			rcode = func(key, val);
			if (!rcode) break;
		}
		*/
	}

	return rcode;
}

bool NodeIceSequence::read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt)
{
	if (!type) return false;
	//assert(opt == 0);
	bool rcode = true;
	v8::HandleScope scope;
	v8::Local<v8::Array> obj(v8::Array::New());
	Ice::Int cnt = s->readSize();
	for (int i = 0; i < cnt; i ++)
	{
		v8::Local<v8::Value> p;
		if (!type->read(s, p, 0))
		{
			rcode = false;
			break;
		}
		obj->Set(i, p);
	}
	p = scope.Close(obj);
	return rcode;
}

void NodeIceDictionary::init(const v8::Arguments& args)
{
	v8::HandleScope scope;
	int argcnt = args.Length();
	int argindex = -1;

	// Process key type
	if (++argindex < argcnt)
		key_type = NodeIceTypeBase::create(args[argindex]);

	// Process value type
	if (++argindex < argcnt)
		value_type = NodeIceTypeBase::create(args[argindex]);
}

bool NodeIceDictionary::write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt)
{
	if (!key_type || !value_type) return false;
	//assert(opt == 0);
	bool rcode = true;
	v8::HandleScope scope;
	v8::Local<v8::Object> obj;	
	v8::Local<v8::Array> keys;
	if (node2obj(p, obj)) keys = obj->GetOwnPropertyNames();
	uint32_t keycnt = keys.IsEmpty() ? 0 : keys->Length();
	s->writeSize((Ice::Int)keycnt);
	for (uint32_t keyno = 0; keyno < keycnt; keyno ++)
	{
		v8::Local<v8::Value> &key = keys->Get(keyno);
		v8::Local<v8::Value> &val = obj->Get(key);
		if (!key_type->write(s, key, 0) || !value_type->write(s, val, 0))
		{
			rcode = false; 
			break; 		
		}
	}
	return rcode;
}

bool NodeIceDictionary::read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt)
{
	if (!key_type || !value_type) return false;
	//assert(opt == 0);
	bool rcode = true;
	v8::HandleScope scope;
	v8::Local<v8::Object> obj(v8::Object::New());
	Ice::Int cnt = s->readSize();
	for (int i = 0; i < cnt; i ++)
	{
		v8::Local<v8::Value> key, value;
		if (!key_type->read(s, key, 0) || !value_type->read(s, value, 0)) 
		{
			rcode = false; 
			break; 		
		}
		obj->Set(key, value);
	}
	p = scope.Close(obj);
	return rcode;
}

void NodeIceParam::init(const v8::Arguments& args)
{
	v8::HandleScope scope;
	int argcnt = args.Length();
	int argindex = -1;

	// Process name
	if (++argindex < argcnt) 
		node2str(args[argindex], name);

	// Process type
	if (++argindex < argcnt)
		type = NodeIceTypeBase::create(args[argindex]);

	// Process optional
	if (++argindex < argcnt)
		node2int(args[argindex], optional);
}

void NodeIceMethod::init(const v8::Arguments& args)
{
	v8::HandleScope scope;
	int len, argcnt = args.Length();
	for(int i = 0; i < argcnt; i ++)
	{
		v8::Local<v8::Value> &val = args[i];
		if (is_empty(val)) continue;

		// Test for object argument
		v8::Local<v8::Object> obj;
		if (node2obj(val, obj))
		{
			// Try process as array of parameters - it`s end of arguments
			v8::Local<v8::Value> &vlen = obj->Get(g_v8s_length);
			if (node2int(vlen, len) && len > 0)
			{
				for (int i = 0; i < len; i ++)
				{
					v8::Local<v8::Value> &param = obj->Get(i);
					NodeIceTypeWrap *wrap = node2_t<NodeIceTypeWrap>(param);
					if (wrap) params.push_back(wrap->type);
					else params.push_back(NodeIceTypePtr(new NodeIceParam(param)));
				}
				break;
			}
		}

		// Process arguments
		switch (i)
		{
		case 0:
			type.reset(new NodeIceParam(val));
			break;
		case 1:
			node2int(val, mode);
			break;
		}
	}
}

bool NodeIceMethod::write_params(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p)
{
	v8::HandleScope scope;
	int cnt = 0;
	bool rcode = true;
	v8::Local<v8::Object> obj;
	node2obj(p, obj);
	s->startEncapsulation();
	std::for_each(params.begin(), params.end(), [&s, &cnt, &obj, &rcode](NodeIceTypePtr &param){
		if (!rcode) return;
		v8::HandleScope scope;
		v8::Local<v8::Value> p;
		if (!obj.IsEmpty()) p = obj->Get(cnt);
		if (!param || !param->write(s, p, 0)) rcode = false;
		cnt ++;
	});
	s->endEncapsulation();
	return rcode;
}

void NodeIceStruct::clear()
{
	std::for_each(items.begin(), items.end(), [](Item *&it){
		if (it) { delete it; it = 0; }
	});
	items.clear();
}

void NodeIceStruct::assign(v8::Handle<v8::Object> &obj)
{
	objenum_t(obj, [this](const v8::Local<v8::Value> &key, v8::Local<v8::Value> &val)->bool{
		this->set(key, val);
		return true;
	});
}

void NodeIceStruct::set(const v8::Local<v8::Value> &key, v8::Local<v8::Value> &val)
{
	NodeIceTypeWrap *wrap = node2_t<NodeIceTypeWrap>(val);
	Item *item = new Item;	
	node2str(key, item->name);
	item->key = v8::Persistent<v8::Value>::New(key);
	if (wrap) item->type = wrap->type;
	else item->type.reset(new NodeIceParam(val));
	items.push_back(item);
}

bool NodeIceStruct::write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt)
{
	v8::HandleScope scope;
	objenumerator objenum(p);
	if (opt != 0 && objenum.empty()) return true; 
	bool rcode = true;

	Ice::OutputStream::size_type spbeg;
	if (opt != 0)
	{
		s->writeOptional(opt, Ice::OptionalFormatFSize);
		s->write(static_cast<Ice::Int>(0));
		spbeg = s->pos();
	}
	std::for_each(items.begin(), items.end(), [&s, &rcode, &objenum](Item *item){
		if (!rcode) return;
		v8::HandleScope scope;
		v8::Local<v8::Value> &p = objenum.next(item->name);
		if (!item->type || !item->type->write(s, p, 0)) rcode = false;
	});
	if (opt != 0)
	{
		s->rewrite(static_cast<Ice::Int>(s->pos() - spbeg), spbeg - 4);
	}
	return rcode;
}

bool NodeIceStruct::read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt)
{
	assert(opt==0);
	bool rcode = true;
	v8::HandleScope scope;
	v8::Local<v8::Object> obj(v8::Object::New());
	std::for_each(items.begin(), items.end(), [&s, &rcode, &obj](Item *item){
		if (!rcode || !item->type) return;
		v8::HandleScope scope;
		v8::Local<v8::Value> p;
		if (!item->type->read(s, p, 0)) rcode = false;
		else obj->Set(item->key, p);
	});
	p = scope.Close(obj);
	return true;
}

//-------------------------------------------------------------------------------------------------------
// NodeIceCommunicator

NodeIceCommunicator::NodeIceCommunicator():
	ic(0)
{
	NODE_DEBUG_MSG("Ice.Communicator::constructor");
}
			
NodeIceCommunicator::~NodeIceCommunicator()
{
	NODE_DEBUG_MSG("Ice.Communicator::destructor");
}


void NodeIceCommunicator::done()
{
	if(ic!=0) ic->destroy();
	ic = 0;
}

/** 
  * this function is called when the communicator function is called as a constructor.
  * usage: var foo = new Communicator("...");
  */
void NodeIceCommunicator::init(const v8::Arguments& args)
{
	NODE_DEBUG_MSG("Ice.Communicator::new");
	options.properties = Ice::createProperties();
	v8::HandleScope scope;	
	v8::Local<v8::Function> func;
	v8::Local<v8::Object> params;
	if (!args2obj(args, 0, params))
	{
		if (!args2func(args, 0, func))
		{
			std::string str;
			if (args2str(args, 0, str)) options.properties->load(str);
		}
	}
	else objenum(params, [this, &func] (std::string &name, v8::Local<v8::Value> &value)->bool 
	{
		std::string str;
		if (name == "config") 
		{
			if (node2str(value, str))
				options.properties->load(str);
		}
		else if (name == "init") 
		{
			node2func(value, func);
		}
		else if (node2str(value, str))
		{
			options.properties->setProperty(name, str);
		}
		return true;
	});

	//if (!func.IsEmpty() || args2func(args, 1, func)) on_init.reset(new NodeCallback(me, func));
	//else on_init.reset();

	ic = Ice::initialize(options);
}

/**
  * done ice communicator
  */
v8::Handle<v8::Value> NodeIceCommunicator::done(const v8::Arguments &args)
{
	NODE_DEBUG_MSG("Ice.Communicator::done");
	v8::HandleScope scope;
	done();
	return scope.Close(v8::Undefined());
}

/**
  * take the string provided as the first argument and return a new Ice Proxy
  */
v8::Handle<v8::Value> NodeIceCommunicator::stringToProxy(const v8::Arguments &args)
{
	NODE_DEBUG_MSG("Ice.Communicator::stringToProxy");
	v8::HandleScope scope;
	std::string str;
	if (!args2str(args, 0, str)) return scope.Close(NODE_ERROR("Ice.Communicator: Invalid arguments"));
	if (!ic) return scope.Close(NODE_ERROR("Ice.Communicator: Invalid instance"));
	Ice::ObjectPrx &prx = ic->stringToProxy(str);
	if (!prx) return scope.Close(NODE_ERROR("Ice.Communicator: Invalid service proxy"));
	v8::Local<v8::Object> proxy = g_proxy->NewInstance();
	NodeIceProxy *p = ObjectWrap::Unwrap<NodeIceProxy>(proxy);
	if (p) p->init(ic, prx);
	return scope.Close(proxy);
}

/**
  * take the property id provided as the first argument and return a new Ice Proxy
  */
v8::Handle<v8::Value> NodeIceCommunicator::propertyToProxy(const v8::Arguments &args)
{
	NODE_DEBUG_MSG("Ice.Communicator::propertyToProxy");
	v8::HandleScope scope;
	std::string str;
	if (!args2str(args, 0, str)) return scope.Close(NODE_ERROR("Ice.Communicator: Invalid arguments"));
	if (!ic) return scope.Close(NODE_ERROR("Ice.Communicator: Invalid instance"));
	Ice::ObjectPrx &prx = ic->propertyToProxy(str);
	if (!prx) return scope.Close(NODE_ERROR("Ice.Communicator: Invalid service proxy"));
	v8::Local<v8::Object> proxy = g_proxy->NewInstance();
	NodeIceProxy *p = ObjectWrap::Unwrap<NodeIceProxy>(proxy);
	if (p) p->init(ic, prx);
	return scope.Close(proxy);
}

//-------------------------------------------------------------------------------------------------------
// NodeIceProxy

NodeIceProxy::NodeIceProxy()
{
	NODE_DEBUG_MSG("Ice.Proxy::constructor");
}

NodeIceProxy::~NodeIceProxy()
{
	NODE_DEBUG_MSG("Ice.Proxy::destructor");
	done();
}

void NodeIceProxy::done()
{
	if (ic) ic = 0;
	if (prx) prx = 0;
}

void NodeIceProxy::init(const v8::Arguments& args)
{
	NODE_DEBUG_MSG("Ice.Proxy::new");
}

v8::Handle<v8::Value> NodeIceProxy::invoke(const v8::Arguments &args)
{
	NODE_DEBUG_MSG("Ice.Proxy::invoke");
	v8::HandleScope scope;

	// Prepare invocation arguments
	int mode = Ice::Normal;
	std::string name;
	NodeIceTypePtr methodptr;
	v8::Local<v8::Value> retinfo;
	v8::Local<v8::Value> arginfo;
	v8::Local<v8::Value> arguments;
	v8::Local<v8::Function> func;
	v8::Local<v8::Object> options;
	if (!args2obj(args, 0, options))
	{
		if (!args2func(args, 0, func))
		{
			args2str(args, 0, name);
		}
	}
	else objenum(options, [&name, &methodptr, &mode, &func, &retinfo, &arginfo, &arguments] (std::string &key, v8::Local<v8::Value> &value)->bool
	{
		if (key == "name") node2str(value, name);
		else if (key == "args") arguments = value;
		else if (key == "arg") arguments = value;
		else if (key == "info") 
		{ 
			NodeIceTypeWrap *wrap = node2_t<NodeIceTypeWrap>(value); 
			if (wrap) methodptr = wrap->type;
		}
		else if (key == "mode") node2int(value, mode);
		else if (key == "result") retinfo = value;
		else if (key == "params") arginfo = value;
		else if (key == "callback") node2func(value, func);
		return true;
	});
	NodeIceMethod *method = dynamic_cast<NodeIceMethod*>(methodptr.get());
	if (method) mode = method->mode;
	if (func.IsEmpty()) args2func(args, 1, func);
	bool async = !func.IsEmpty();

	// Prepare invocation result
	IceResult *result = new IceResult();
	result->holder = v8::Persistent<v8::Object>::New(args.Holder());
	if (async) result->ptr.reset(new NodeCallback(args.This(), func));
	if (method) result->type = method->type;
	else if (!retinfo.IsEmpty()) result->info = v8::Persistent<v8::Value>::New(retinfo);
	IceCallbackPtr resultptr(new IceCallback(result));

	// Begin invocation asynchronically
	std::string err;
	try 
	{
		/*
		std::vector<Ice::Byte> inParams2;
		std::vector<Ice::Byte> &inParams = inParams2;
		Ice::OutputStreamPtr &s2 = Ice::createOutputStream(ic);
		s2->startEncapsulation();
		s2->write("test");

		//s2->write(Ice::Int(5000));

		s2->write(1, IceUtil::Optional<Ice::Int>(5000));
		//s2->writeOptional(1, Ice::OptionalFormatF4);
		//s2->write(Ice::Int(5000));

			if (s2->writeOptional(1, Ice::OptionalFormatFSize))
			{
				s2->write(static_cast<Ice::Int>(0));
				Ice::OutputStream::size_type p = s2->pos();
				s2->write(Ice::Long(127));
				s2->write("guest");
				s2->write(Ice::Double(1));
				s2->rewrite(static_cast<Ice::Int>(s2->pos() - p), p - 4);
			}

		s2->endEncapsulation();
		s2->finished(inParams2);
		/*/

		//*
		std::vector<Ice::Byte> inParams;
		if (method)
		{
			if (method->IsArguments())
			{
				Ice::OutputStreamPtr &s = Ice::createOutputStream(ic);
				if (method->write_params(s, arguments)) 
					s->finished(inParams);
			}
		}
		else if (!is_empty(arginfo))
		{
			Ice::OutputStreamPtr &s = Ice::createOutputStream(ic);
			if (node2ice(s, arginfo, arguments)) 
				s->finished(inParams);
		}
		//*/

		if (async)
		{
			Ice::CallbackPtr cbptr = Ice::newCallback(resultptr, &IceCallback::process);
			prx->begin_ice_invoke(name, Ice::OperationMode(mode), inParams, cbptr);
		}
		else
		{
			Ice::AsyncResultPtr &r = prx->begin_ice_invoke(name, Ice::OperationMode(mode), inParams);
			r->waitForCompleted();
			resultptr->do_process(r, false);
		}
	}
	catch (const Ice::UserException &e) { EXCEPTION_FORMAT(err, "Ice User ", e); }
	catch (const Ice::LocalException &e) { EXCEPTION_FORMAT(err, "Ice Local ", e); }
	catch (const std::exception &e) { EXCEPTION_FORMAT(err, "", e); }
	catch (...) { err = "Unknown Exception"; }

	// Process result
	if (!err.empty()) return scope.Close(NODE_ERROR(err.c_str()));
	if (async) return scope.Close(v8::Undefined());
	if (!result->err.empty()) return scope.Close(NODE_ERROR(result->err.c_str()));
	return scope.Close(result->value);
}

//-------------------------------------------------------------------------------------------------------