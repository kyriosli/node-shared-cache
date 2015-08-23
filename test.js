var assert = require('assert');

var binding = require('./index.js');
/*
try {
    var obj = new binding.Cache("test", 525312);
} catch (e) {
    console.error(e);
}

try {
    var obj = new binding.Cache("test", 4294967295);
} catch (e) {
    console.error(e);
}
*/

var obj = new binding.Cache("test", 512<<10, binding.SIZE_1K);
console.log('set obj.foo');
obj.foo = "bar";

assert.strictEqual(obj.foo, "bar");

obj.env = process.env;

// free block
obj.env = 0;

// increase block
obj.env = [process.env, process.env];

assert.deepEqual(Object.keys(obj).slice(-2), ['foo', 'env']);

for(var k in obj) {
    console.log(k, obj[k]);
}

var test = [process.env, process.env];

test[2] = {'test':test};
obj.env = test;

test = obj.env;
assert.strictEqual(test, test[2].test);
assert.strictEqual(test[0], test[1]);

delete obj.foo;
assert.ifError('foo' in obj);
assert.strictEqual(obj.foo, undefined);


console.time('LRU cache replacement');
for(var i = 0; i < 1000000; i+=3) {
    obj['test' + i] = i;
    assert.strictEqual(obj['test' + i] , i);
}
console.timeEnd('LRU cache replacement');
assert.ifError('test0' in obj);

var longData = Array(15).join(Array(64).join('abcdefgh')); // 17 blocks

obj.test = longData;
assert.strictEqual(obj.test, longData);
