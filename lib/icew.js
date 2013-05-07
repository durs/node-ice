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
	* Ice Communicator wrapper
	*/
	var Communicator = this.Communicator = function(opt){
		this._base = new base.Communicator(opt);
		if (!this._base) throw Error('Invalid Ice.Communicator object');
		return this;
		/*
		if (typeof(opt)!='object') opt = {}
		var me = this;
		var f_init = opt.init;
		opt.init = function(){
			me.initialized = true;
			if (f_init) f_init.apply(this, arguments);
		}
		base.init(opt);
		*/
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
	var new_func = function(name, opt){
		return function(){
		
			// Prepare callback - back argument
			var argcnt = arguments.length;
			var cb = (argcnt>0) ? arguments[argcnt-1] : null;
			if (typeof(cb)=='function') argcnt--; else cb=null;			
			
			// Invoke method
			return this.invoke({
				name: name,
				mode: opt.mode,
				params: opt.args,
				args: arguments,
				result: opt.result,
				callback: cb
			});
		}	
	}	
	var Proxy = function(base, props){
		this._base = base;
		for(var key in props){
			var val = props[key];
			if (typeof(val)=='object'){
				this[key] = new_func(key, val);
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
