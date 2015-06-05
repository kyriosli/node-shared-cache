var cp = require('child_process');

if(process.argv[2] !== 'worker') {
	var workers = [];
	for(var i = 0; i < 10; i++) {
		var worker = workers[i] = cp.spawn(process.execPath, [__filename, 'worker'], {stdio: 'inherit'});
		console.log('spawn child ', i, worker.pid);
	}
	process.on('SIGINT', function () {
		for(var i = 0; i < 10; i++) {
			workers[i].kill('SIGINT');
		}
	});

	return;
}

var n = 0;
var binding = require('./build/Release/binding.node');
var obj = new binding.Cache("test", 557056);

function sched() {
	n++;
	switch(Math.random() * 10 | 0) {
		case 0: obj.foo = !obj.foo; break;
		case 1: obj.foo = Date.now() + Math.random(); break;
		case 2: obj.foo = process.env; break;
		case 3: delete obj.foo; break;
		case 4: Object.keys(obj); break;
		case 5: obj.foo = null; break;
		default: obj.foo; break;
	}
	setTimeout(sched, 0);
}

sched();

process.on('SIGINT', function () {
	console.log(process.pid, n);
	process.exit();
});