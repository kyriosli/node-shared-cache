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

var obj = new binding.Cache("test", 512<<10, binding.SIZE_1K);
binding.clear(obj);

console.log('set obj.foo');
obj.foo = "bar";

assert.strictEqual(obj.foo, "bar");

obj.env = process.env;

// free block
obj.env = 0;

var nested = [process.env, process.env, {}];
obj.nested = nested;

console.log(binding.dump(obj));

assert.deepEqual(binding.dump(obj), {foo: 'bar', env: 0, nested: nested})

nested[2].test = nested;
// increase block
obj.nested = nested;

assert.deepEqual(Object.keys(obj), ['foo', 'env', 'nested']);

var result = obj.nested;
assert.strictEqual(result, result[2].test);
assert.strictEqual(result[0], result[1]);

for(var k in obj) {
    console.log(k, obj[k]);
}


obj.foo2 = 1234;
assert.deepEqual(binding.dump(obj, 'foo'), {foo: 'bar', foo2: 1234});
assert.deepEqual(binding.dump(obj, 'e'), {env: 0});

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
