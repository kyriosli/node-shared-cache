var assert = require('assert');

var binding = require('./build/Release/binding.node');

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

var obj = new binding.Cache("test", 557056);

obj.foo = "bar";

console.log(obj.foo);


obj.env = process.env;

// free block
obj.env = 0;

// increase block
obj.env = [process.env, process.env]

assert.deepEqual(Object.keys(obj), ['foo', 'env']);

for(var k in obj) {
	console.log(k, obj[k]);
}

var test = [process.env, process.env];

test[2] = {'test':test};
obj.env = test;

console.log(obj.env);

test = obj.env;
assert.strictEqual(test, test[2].test);

delete obj.foo;
console.log('foo' in obj);
assert.ifError('foo' in obj);
assert.strictEqual(obj.foo, undefined);