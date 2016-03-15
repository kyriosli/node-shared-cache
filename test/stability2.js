var binding = require('../index.js');
var obj = new binding.Cache("test", 512<<10, binding.SIZE_1K);

var key = 'foo';

for(var i = 0; ; i++) {
    switch(Math.random() * 10 | 0) {
        case 0: obj[key] = !obj[key]; break;
        case 1: obj[key] = Date.now() + Math.random(); break;
        case 2: obj[key] = process.env; break;
        case 3: delete obj[key]; break;
        case 4: Object.keys(obj); break;
        case 5: obj[key] = null; break;
        case 6: key in obj; break;
        case 7: key = 'foo' + (i & 7);
        default: obj[key]; break;
    }
    (i & 4095) || console.log(i);
}