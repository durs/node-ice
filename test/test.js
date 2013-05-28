/**
 * @author Yuri Dursin
 * @description ZeroC Ice wrapper test script
 */

var ice = require('../index.js');

//--------------------------------------------------------------------------
// Monitoring Slice info (javascript analog)

/*
module Monitoring
{
	sequence<byte> ByteArray;

	struct Options
	{
		long params;
		string userid;
		double modifytime;
	};

    struct Item
	{
		string id;
		double modifytime;
		Properties props;
	};

	sequence<Item> ItemArray;

	interface Service
	{
		void shutdown();
		idempotent string echo(string msg, optional(1) int delay, optional(2) Options opt);
		idempotent Options testStruct();
		idempotent IntValue testClass();
		idempotent ByteArray testSequence();
		idempotent Properties testDictionary();
		idempotent void testException(int code, string msg) throws Error;
	};

    interface Storage
	{
        ["ami", "amd", "cpp:const"] idempotent string info() throws Error;
        ["ami", "amd"] idempotent ItemArray get(string name, Options opt) throws Error;			
        ["ami", "amd"] idempotent string format(string name, Options opt, string fmt) throws Error;			
        ["ami", "amd"] idempotent Item getItem(string name, string itemid) throws Error;			

		void addObserver(Ice::Identity id);
	};
}
*/

var Monitoring = (function () {

    var ByteArray = ice.Sequence('byte');

    var Options = {
        params: 'long',
        userid: 'string',
        modifytime: 'double'
    }

    var Value = {
        type: 'short',
        val: 'string'
    }

    var Properties = ice.Dictionary('string', Value);

    var Item = {
		id: 'string',
		modifytime: 'double',
		props: Properties
    }

    var ItemArray = ice.Sequence(Item);

    var Service = {
        shutdown: ice.Method(),
        echo: ice.Method('string', ice.mode.idempotent, [
			ice.Argument('msg', 'string'),
			ice.Argument('delay', 'int', 1),
			ice.Argument('opt', Options, 2)
		]),
        testStruct: ice.Method(Options, ice.mode.idempotent),
        testClass: ice.Method(null, ice.mode.idempotent, ['int', 'string']),
        testSequence: ice.Method(ByteArray, ice.mode.idempotent),
        testDictionary: ice.Method(Properties, ice.mode.idempotent),
        testException: ice.Method(null, ice.mode.idempotent, ['int', 'string'])
    }

    var Storage = {
        info: ice.Method('string', ice.mode.idempotent),
        get: ice.Method(ItemArray, ice.mode.idempotent, [
            ice.Argument('name', 'string'),
            ice.Argument('opt', Options)
        ]),
        format: ice.Method('string', ice.mode.idempotent, [
            ice.Argument('name', 'string'),
            ice.Argument('opt', Options),
            ice.Argument('fmt', 'string')
        ])
    }

    return {
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

function srv_test() {
    //var ret = srv.testStruct();
    //var ret = srv.testSequence();
    //var ret = srv.testDictionary();
    var ret = srv.testException(1, "no message");
    message('Service test: ' + (ret ? ret.toString() : 'undefined'));
}

function srv_echo(){
    message('Service echo: ' + srv.echo('test', null, { params: 127, userid: "guest", modifytime: 1 }));
}

function strg_format(){
	message('Storage format: ' + strg.format('test.users', null, 'json'));
}

function strg_get() {
    var items = strg.get('test.users');
    var len = items.length;
    message('Storage items: ' + len);
    for (var i = 0; i < len; i++) {
        var item = items[i] || {};
        var info = '>>>' + item.id;
        if (item.props) info += ' name: ' + item.props['name'].val;
        message(info);
    }
}

function strg_info(async) {
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
    if (cmd == 'test\r\n') test_call(function () { srv_test(); });
	else if (cmd=='format\r\n') test_call(function(){ strg_format(); });
	else if (cmd=='items\r\n') test_call(function () { strg_get(); });
	else if (cmd=='info\r\n') test_call(function () { strg_info(false); });
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
	'format: storage test\r\n'+
	'items: storage test\r\n' +
	'info: sync query storage info\r\n' +
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
