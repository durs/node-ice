#ifndef _node_ice_h__
#define _node_ice_h__

#include "nodeutils.h"

#include <Ice/Ice.h>
#include <Ice/Service.h>
#include <IceUtil/Thread.h>


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
	//static v8::Handle<v8::Value> StringToProxy(const v8::Arguments &args);
	//static v8::Handle<v8::Value> PropertyToProxy(const v8::Arguments &args);
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


void ice_register(v8::Handle<v8::Object> target);

//-------------------------------------------------------------------------------------
#endif