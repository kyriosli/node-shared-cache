var assert = require('assert');

var binding = require('../index.js');
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

var obj = new binding.Cache("test2", 512<<10, binding.SIZE_64);


obj.foo = "bar";

assert.strictEqual(obj.foo, "bar");

obj.env = process.env;

// free block
obj.env = 0;

// increase block
obj.env = [process.env, process.env];

assert.deepEqual(Object.keys(obj).slice(-2), ['foo', 'env']);

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
for(var i = 0; i < 4097; i++) {
    obj['test' + i] = i;
    assert.strictEqual(obj['test' + i] , i);
}
console.timeEnd('LRU cache replacement');
assert.ifError('test0' in obj);

var longData = Array(8192).join('abcdefgh');

obj.test = longData;
assert.strictEqual(obj.test, longData);
