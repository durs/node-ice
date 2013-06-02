#ifndef _node_ice_h__
#define _node_ice_h__

#include "nodeutils.h"

#include <Ice/Ice.h>
#include <Ice/Service.h>
#include <IceUtil/Thread.h>

class NodeIceCommunicator;
class NodeIceProxy;

//-------------------------------------------------------------------------------------

#define ICE2NODE(p, s, t1, t2, t3) { t1 v; s->read(v); p=v8::t2::New((t3)v); }
#define ICE2BOOL(p, s) ICE2NODE(p, s, bool, Boolean, bool)
#define ICE2BYTE(p, s) ICE2NODE(p, s, Ice::Byte, Int32, long)
#define ICE2SHORT(p, s) ICE2NODE(p, s, Ice::Short, Int32, long)
#define ICE2INT(p, s) ICE2NODE(p, s, Ice::Int, Int32, long)
#define ICE2LONG(p, s) ICE2NODE(p, s, Ice::Long, Integer, long)
#define ICE2FLOAT(p, s) ICE2NODE(p, s, Ice::Float, Number, double)
#define ICE2DOUBLE(p, s) ICE2NODE(p, s, Ice::Double, Number, double)
#define ICE2STR(p, s) { std::wstring v; s->read(v); p=v8::String::New((uint16_t*)v.c_str(), v.size()); }

#define ICE3NODE(p, s, t1, t2, def) { t1 v; \
	try{ v=p.IsEmpty()?def:(t1)p->t2##Value(); }catch(...){ v=def; } \
	s->write(v); }
#define ICE3BOOL(p, s) ICE3NODE(p, s, bool, Boolean, false)
#define ICE3BYTE(p, s) ICE3NODE(p, s, Ice::Byte, Int32, 0)
#define ICE3SHORT(p, s) ICE3NODE(p, s, Ice::Short, Int32, 0)
#define ICE3INT(p, s) ICE3NODE(p, s, Ice::Int, Int32, 0)
#define ICE3LONG(p, s) ICE3NODE(p, s, Ice::Long, Integer, 0)
#define ICE3FLOAT(p, s) ICE3NODE(p, s, Ice::Float, Number, 0)
#define ICE3DOUBLE(p, s) ICE3NODE(p, s, Ice::Double, Number, 0)
#define ICE3STR(p, s) { std::wstring v; node2str(p, v); s->write(v); }

inline Ice::Byte node2ice(v8::Local<v8::Value> &p, const Ice::Byte def) { return p.IsEmpty() ? def : (Ice::Byte)p->Int32Value(); }
inline Ice::Short node2ice(v8::Local<v8::Value> &p, const Ice::Short def) { return p.IsEmpty() ? def : (Ice::Short)p->Int32Value(); }
inline Ice::Int node2ice(v8::Local<v8::Value> &p, const Ice::Int def) { return p.IsEmpty() ? def : (Ice::Int)p->Int32Value(); }
inline Ice::Long node2ice(v8::Local<v8::Value> &p, const Ice::Long def) { return p.IsEmpty() ? def : (Ice::Long)p->IntegerValue(); }
inline Ice::Float node2ice(v8::Local<v8::Value> &p, const Ice::Float def) { return p.IsEmpty() ? def : (Ice::Float)p->NumberValue(); }
inline Ice::Double node2ice(v8::Local<v8::Value> &p, const Ice::Double def) { return p.IsEmpty() ? def : (Ice::Double)p->NumberValue(); }
inline bool node2ice(v8::Local<v8::Value> &p, const bool def) { return p.IsEmpty() ? def : p->BooleanValue(); }
inline std::wstring node2ice(v8::Local<v8::Value> &p, const std::wstring &def) { std::wstring v; if (!node2str(p, v)) v = def; return v; }

inline v8::Local<v8::Value> ice2node(const Ice::Byte v) { v8::HandleScope scope; return scope.Close(v8::Int32::New(v)); }
inline v8::Local<v8::Value> ice2node(const Ice::Short v) { v8::HandleScope scope; return scope.Close(v8::Int32::New(v)); }
inline v8::Local<v8::Value> ice2node(const Ice::Int v) { v8::HandleScope scope; return scope.Close(v8::Int32::New(v)); }
inline v8::Local<v8::Value> ice2node(const Ice::Long v) { v8::HandleScope scope; return scope.Close(v8::Integer::New(v)); }
inline v8::Local<v8::Value> ice2node(const Ice::Float v) { v8::HandleScope scope; return scope.Close(v8::Number::New(v)); }
inline v8::Local<v8::Value> ice2node(const Ice::Double v) { v8::HandleScope scope; return scope.Close(v8::Number::New(v)); }
inline v8::Local<v8::Value> ice2node(const bool v) { v8::HandleScope scope; return scope.Close(v8::Boolean::New(v)); }
inline v8::Local<v8::Value> ice2node(const std::wstring &v) { v8::HandleScope scope; return scope.Close(v8::String::New((uint16_t*)v.c_str(), v.length())); }

//-------------------------------------------------------------------------------------

class NodeIceTypeBase;
typedef boost::shared_ptr<NodeIceTypeBase> NodeIceTypePtr;

class NodeIceTypeBase
{
public:
	static NodeIceTypePtr create(v8::Handle<v8::Value> &info);

	virtual ~NodeIceTypeBase() {}

	virtual void init(const v8::Arguments& args);
	virtual void set(const v8::Local<v8::Value> &key, v8::Local<v8::Value> &val) {}
	virtual bool write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt) { assert(false); return false; }
	virtual bool read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt) { assert(false); return false; }
};

template<typename T>
class NodeIceSimpleT: public NodeIceTypeBase
{
public:
	static const T defval;
	virtual bool write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt);
	virtual bool read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt);
};

class NodeIceSequence: public NodeIceTypeBase
{
public:
	inline NodeIceSequence() {}
	inline NodeIceSequence(const NodeIceSequence &v): type(v.type) {}
	virtual ~NodeIceSequence() {}

	virtual void init(const v8::Arguments& args);
	virtual bool write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt);
	virtual bool read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt);

private:
	NodeIceTypePtr type;
};

class NodeIceDictionary: public NodeIceTypeBase
{
public:
	inline NodeIceDictionary() {}
	inline NodeIceDictionary(const NodeIceDictionary &v): key_type(v.key_type), value_type(v.value_type) {}
	virtual ~NodeIceDictionary() {}

	virtual void init(const v8::Arguments& args);
	virtual bool write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt);
	virtual bool read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt);

private:
	NodeIceTypePtr key_type;
	NodeIceTypePtr value_type;
};

class NodeIceParam: public NodeIceTypeBase
{
public:
	inline NodeIceParam(): optional(0) {}
	inline NodeIceParam(const NodeIceParam &v): name(v.name), type(v.type), optional(v.optional) {}
	inline NodeIceParam(v8::Handle<v8::Value> &type_info): type(NodeIceTypeBase::create(type_info)), optional(0) {}

	virtual void init(const v8::Arguments& args);
	virtual bool write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt) { return (type!=0) ? type->write(s, p, optional) : false; }
	virtual bool read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt) { return (type!=0) ? type->read(s, p, optional) : false; }

private:
	std::wstring name;
	NodeIceTypePtr type;
	int optional;
};

class NodeIceMethod: public NodeIceTypeBase
{
public:
	typedef std::vector<NodeIceTypePtr> Params;
	NodeIceTypePtr type;
	Params params;
	int mode;

	inline NodeIceMethod() {}
	inline bool IsArguments() { return !params.empty(); }
	bool write_params(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p);

	virtual void init(const v8::Arguments& args);
	virtual bool write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt) { return true; }
	virtual bool read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt) { return true; }
};

class NodeIceStruct: public NodeIceTypeBase
{
public:
	inline NodeIceStruct() {}
	inline NodeIceStruct(v8::Handle<v8::Object> &obj) { assign(obj); }
	virtual ~NodeIceStruct() { clear(); }
	void clear();

	virtual void set(const v8::Local<v8::Value> &key, v8::Local<v8::Value> &val);
	virtual bool write(Ice::OutputStreamPtr &s, v8::Local<v8::Value> &p, int opt);
	virtual bool read(Ice::InputStreamPtr &s, v8::Local<v8::Value> &p, int opt);

private:
	struct Item
	{
		std::wstring name;
		v8::Persistent<v8::Value> key;
		NodeIceTypePtr type;
	};
	typedef std::vector<Item*> Items; 
	Items items;
	void assign(v8::Handle<v8::Object> &obj);
};

//-------------------------------------------------------------------------------------

class NodeIceTypeWrap: public node::ObjectWrap
{
public:
	NodeIceTypePtr type;
};

template<typename T>
class NodeIceTypeT: public NodeIceTypeWrap
{
public:
	static void node_register(v8::Handle<v8::Object> &target, const char *name, bool as_instance);
	NODE_DEFINE_CONSTRUCTOR(NodeIceTypeT, init)
};

//-------------------------------------------------------------------------------------

class NodeIceCommunicator: 
	public node::ObjectWrap 
{
public:
			
	NodeIceCommunicator();			
	~NodeIceCommunicator();

	Ice::CommunicatorPtr ic;
	Ice::InitializationData options;
	void done();

	NODE_DEFINE_CONSTRUCTOR(NodeIceCommunicator, init)
	NODE_DEFINE_METHOD(NodeIceCommunicator, done)
	NODE_DEFINE_METHOD(NodeIceCommunicator, stringToProxy)
	NODE_DEFINE_METHOD(NodeIceCommunicator, propertyToProxy)
};

//-------------------------------------------------------------------------------------

class NodeIceProxy: 
	public node::ObjectWrap
{
public:

	NodeIceProxy();
	~NodeIceProxy();

	Ice::CommunicatorPtr ic;
	Ice::ObjectPrx prx;
	void done();

	inline void init(Ice::CommunicatorPtr &ic_, Ice::ObjectPrx &prx_) { ic = ic_; prx = prx_; }

	NODE_DEFINE_CONSTRUCTOR(NodeIceProxy, init)
	NODE_DEFINE_METHOD(NodeIceProxy, invoke)
	//NODE_DEFINE_METHOD(NodeIceProxy, value_of)
	//NODE_DEFINE_METHOD(NodeIceProxy, to_string)

};


void ice_node_register(v8::Handle<v8::Object> &target);

//-------------------------------------------------------------------------------------
#endif