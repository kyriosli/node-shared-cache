var assert = require('assert');

var binding = require('./build/Release/binding.node');
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

var obj = new binding.Cache("test", 1048576);
obj.foo = "bar";

assert.strictEqual(obj.foo, "bar");

obj.env = process.env;

// free block
obj.env = 0;

// increase block
obj.env = [process.env, process.env]

assert.deepEqual(Object.keys(obj).slice(-2), ['foo', 'env']);

for(var k in obj) {
//  console.log(k, obj[k]);
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

