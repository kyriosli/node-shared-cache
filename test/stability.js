var cp = require('child_process');

if(process.argv[2] !== 'worker') {
    var workers = [], spawned = 0;
    for(var i = 0; i < 10; i++) {
        spawned++;
        var worker = workers[i] = cp.spawn(process.execPath, [__filename, 'worker'], {stdio: 'inherit'});
    }
    process.on('SIGINT', function () {
        for(var i = 0; i < 10; i++) {
            workers[i].kill('SIGINT');
        }
        clearInterval(pid);
        console.log(spawned + ' children spawned');
    });

    var pid = setInterval(function() {
        var i = Math.random() * 10 | 0;
        workers[i].kill('SIGKILL');
        spawned++;
        var worker = workers[i] = cp.spawn(process.execPath, [__filename, 'worker'], {stdio: 'inherit'});
    }, 100);

    return;
}

var n = 0, stopped = false;
var binding = require('../index.js');
var obj = new binding.Cache("test", 512<<10, binding.SIZE_1K);


process.on('SIGINT', function () {
    console.log(process.pid, n);
    stopped = true;
});

function sched() {
    n++;
    for(var i = 0; i < 1000; i++) {
        switch(Math.random() * 10 | 0) {
            case 0: obj.foo = !obj.foo; break;
            case 1: obj.foo = Date.now() + Math.random(); break;
            case 2: obj.foo = process.env; break;
            case 3: delete obj.foo; break;
            case 4: Object.keys(obj); break;
            case 5: obj.foo = null; break;
            case 6: 'foo' in obj; break;
            default: obj.foo; break;
        }
    }
    stopped || setTimeout(sched, 0);
}

sched();
