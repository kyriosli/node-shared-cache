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

delete obj.foo;
'foo' in obj;
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