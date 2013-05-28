/**
 * @author Yuri Dursin
 * @description ZeroC Ice wrapper
 */

//--------------------------------------------------------------------------

var base = require('./ice');
if (!base) throw Error('Invalid Ice object'); 

//--------------------------------------------------------------------------

var ice = (function(){

	var comm = null;

	/**
	* Ice constants
	*/
	this.mode = { normal: 0, nonmutating: 1, idempotent: 2 }
	
	/**
	* Ice simple initialization
	*/
	this.initialized = function(){
		return comm != null;
	}
	this.communicator = function(){
		return comm;
	}
	this.init = function(opt){
		this.done();
		return comm = new Communicator(opt);
	}
	this.done = function(){
		if (comm) comm.done();
		comm = null;
	}
	this.stringToProxy = function(name){
		return comm ? comm.stringToProxy(name) : null;
	}
	this.propertyToProxy = function(name){
		return comm ? comm.propertyToProxy(name) : null;
	}

	/**
	* Ice types
	*/
	this.Byte = base.Byte;
	this.Int = base.Int;
	this.Long = base.Long;
	this.Double = base.Double;
	this.Float = base.Float;
	this.Bool = base.Bool;
	this.Str = base.String; // this.String genrate error, why?
	this.Sequence = function (type) { return new base.Sequence(type); }
	this.Dictionary = function (key_type, value_type) { return new base.Dictionary(key_type, value_type); }
	this.Field = function (name, type, optional) { return new base.Field(name, type, optional); }
	this.Argument = function (name, type, optional) { return new base.Argument(name, type, optional); }
	this.Method = function (type, kind, params) { return new base.Method(type, kind, params); }
	this.Struct = function (info) { return new base.Struct(info); }
	//this.Class = function (info) { return new base.Class(info); }
	//this.Interface = function (info) { return new base.Interface(info); }
	//this.Exception = function (info) { return new base.Exception(info); }

	/**
	* Ice Communicator wrapper
	*/
	var Communicator = this.Communicator = function(opt){
		this._base = new base.Communicator(opt);
		if (!this._base) throw Error('Invalid Ice.Communicator object');
		return this;
	}
	Communicator.prototype.done = function(){
		if (this._base) this._base.done();
		this._base = null;
	}
	Communicator.prototype.stringToProxy = function(opt, props){
		if (!this._base) return null;
		var p = this._base.stringToProxy(opt);
		if (!p) return null;
		return new Proxy(p, props);
	}
	Communicator.prototype.propertyToProxy = function(opt, props){
		if (!this._base) return null;
		var p = this._base.propertyToProxy(opt);
		if (!p) return null;
		return new Proxy(p, props);
	}

	/**
	* Ice Proxy wrapper
	*/
	var new_func = function(name, info){
		return function(){
		
			// Prepare callback - back argument
			var argcnt = arguments.length;
			var cb = (argcnt>0) ? arguments[argcnt-1] : null;
			if (typeof(cb)=='function') argcnt--; else cb=null;			
			
			// Invoke method
			return this.invoke({
				name: name,
				args: arguments,
				info: info,
				mode: info.mode,
				params: info.args,
				result: info.result,
				callback: cb
			});
		}
	}
	var Proxy = function(base, props){
		this._base = base;
		for(var key in props){
			var info = props[key];
			if (typeof(info)=='object'){
				this[key] = new_func(key, info);
			}
		}
		return this;
	}
	Proxy.prototype.invoke = function(opt){
		return this._base.invoke(opt);
	}
	
	return this;
})();

//--------------------------------------------------------------------------

module.exports = ice;

//--------------------------------------------------------------------------
