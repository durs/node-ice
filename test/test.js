/**
 * @author Yuri Dursin
 * @description ZeroC Ice wrapper test script
 */

var ice = require('../index.js');

//--------------------------------------------------------------------------
// Monitoring Slice info

/*
	sequence<byte> ByteArray;

	struct Options
	{
		long params;
		string userid;
		double modifytime;
	};

	interface Service
	{
		void shutdown();
		idempotent string echo(string msg, optional(1) int delay, optional(2) Options opt);
	};

    interface Storage
	{
        ["ami", "amd", "cpp:const"] idempotent string info() throws Error;
        ["ami", "amd"] idempotent ItemArray get(string name, Options opt) throws Error;			
        ["ami", "amd"] idempotent string format(string name, Options opt, string fmt) throws Error;			
        ["ami", "amd"] idempotent Item getItem(string name, string itemid) throws Error;			

		void addObserver(Ice::Identity id);
	};
*/

var Monitoring = (function(){
	var Error = {
		code: 'long',
        message: 'string'
	}
	var ByteArray = ice.Sequence('byte');
	var Options = {
		params: 'long',
        userid: 'string',
        modifytime: 'double'
	}
    var Service = {
        shutdown: ice.Method(),
		echo: ice.Method('string', ice.mode.idempotent, [
			ice.Argument('msg', 'string'), 
			ice.Argument('delay', 'int', 1),
			ice.Argument('opt', Options, 2)
		//ice.Argument('opt', ByteArray, 1)
		])
	}
	var Storage = {
		info: { mode: ice.mode.idempotent, result: 'string', error: Error },
		format: { mode: ice.mode.idempotent, result: 'string', error: Error, args:{
			name: 'string',
			opt: Options,
			fmt: 'string'
		}}
	}
	return {
		Error: Error,
		Options: Options,
		Storage: Storage,
		Service: Service
	}
})();

//--------------------------------------------------------------------------
// Tests

var ic;
var strg, srv;
function test_call(func){
	try{ 
		if (!ic) ic = new ice.Communicator('./test.conf');
		if (!strg) strg = ic.propertyToProxy('Storage.Proxy', Monitoring.Storage);
		if (!srv) srv = ic.propertyToProxy('Service.Proxy', Monitoring.Service);
		
		//if (!ice.initialized()) ice.init('./test.conf');
		//if (!strg) strg = ice.communicator().propertyToProxy('Storage.Proxy');	
		
		var t = new Date();
		if (strg) func(); 	
		var src = (new Date() - t) / 1000;
		console.log('### process ' + src.toFixed(2) + ' sec');
	}
	catch(e){ 
		message('exception: ' + e.message); 
	}
}

function srv_echo(){
    message('Service echo: ' + srv.echo('test', null, { params: 127, userid: "guest", modifytime: 1 }));
}

function strg_format(){
	message('Storage format: ' + strg.format('test.users', null, 'json'));
}

function strg_info(async){
	if (!async) message('Storage info: ' + strg.info());
	else strg.info(function(err, result){
		if (err) message('Storage async error: ' + err, true);
		else message('Storage async info: ' + result, true);
	});
}

var stop_loop;
function strg_info_loop(){
	var cnt = 0;
	stop_loop = false;
	var f = function(){
		message('=== ' + (++cnt) + ' ===');
		strg_info(false);
		strg_info(true);
		if (!stop_loop) setTimeout(f,1);
	}
	f();
}

//--------------------------------------------------------------------------
// Main

function exec(cmd){
	if (cmd=='test\r\n') test_call(function(){ srv_echo(); });
	else if (cmd=='format\r\n') test_call(function(){ strg_format(); });
	else if (cmd=='info\r\n') test_call(function(){ strg_info(false); });
	else if (cmd=='ainfo\r\n') test_call(function(){ strg_info(true); });
	else if (cmd=='dinfo\r\n') test_call(function(){ strg_info(true); strg_info(false); });
	else if (cmd=='start\r\n') test_call(function(){ strg_info_loop(); });
	else if (cmd=='stop\r\n') stop_loop = true;
	else if (cmd=='exit\r\n') process.exit();
	else if (cmd=='?\r\n') help();
	//if (cmd='')
}
function help(){
	console.log(
	'-----------------------------------------------\r\n'+
	'Script usage help:\r\n\r\n'+
	'-----------------------------------------------\r\n'+
	'test: run common test\r\n'+
	'format: format storage test\r\n'+
	'info: sync query storage info\r\n'+
	'ainfo: async query storage info\r\n'+
	'dinfo: async after sync (double) query storage info\r\n'+
	'start: start loop any sync/async calls (test memory leaks)\r\n'+
	'stop: stop loop\r\n'+
	'exit: exit\r\n'+
	'?: help\r\n'+
	'-----------------------------------------------'
	);
}
function message(text, async){
	var str = '';
	if (async) str += '\r\n>>>';
	str += text + '\r\n';
	if (async) str +='==>';
	process.stdout.write(str);
}
function wait(){
	process.stdout.write('==>');
	process.stdin.on('data', function(cmd){
		exec(cmd);
		process.stdout.write('==>');
	});
}
process.stdin.resume();
process.stdin.setEncoding('utf8');
help();
wait();


//--------------------------------------------------------------------------
