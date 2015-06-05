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

// test plain object
var plain = {};
console.time('plain obj');
for(var i = 0; i < 1000000; i++) {
    plain['test' + (i & 127)] = i;
}
console.timeEnd('plain obj');

// test shared cache
var obj = new binding.Cache("test", 1048576);
console.time('shared cache');
for(var i = 0; i < 1000000; i++) {
    obj['test' + (i & 127)] = i;
}
console.timeEnd('shared cache');

// test read existing key
console.time('read plain obj');
for(var i = 0; i < 1000000; i++) {
    plain['test' + (i & 127)];
}
console.timeEnd('read plain obj');

console.time('read shared cache');
for(var i = 0; i < 1000000; i++) {
    obj['test' + (i & 127)];
}
console.timeEnd('read shared cache');

console.time('read plain obj with key absent');
for(var i = 0; i < 1000000; i++) {
    plain['oops' + (i & 127)];
}
console.timeEnd('read plain obj with key absent');

console.time('read shared cache with key absent');
for(var i = 0; i < 1000000; i++) {
    obj['oops' + (i & 127)];
}
console.timeEnd('read shared cache with key absent');

// test enumerating keys
console.time('enumerate plain obj');
for(var i = 0; i < 100000; i++) {
    Object.keys(plain);
}
console.timeEnd('enumerate plain obj');

console.time('enumerate shared cache');
for(var i = 0; i < 100000; i++) {
    Object.keys(obj);
}
console.timeEnd('enumerate shared cache');

// test object serialization
var input = {env: process.env, arr: [process.env, process.env]};
console.time('JSON.stringify');
for(var i = 0; i < 100000; i++) {
    JSON.stringify(input);
}
console.timeEnd('JSON.stringify');

console.time('binary serialization');
for(var i = 0; i < 100000; i++) {
    obj.test = input;
}
console.timeEnd('binary serialization');

// test object unserialization
input = JSON.stringify(input);
console.time('JSON.parse');
for(var i = 0; i < 100000; i++) {
    JSON.parse(input);
}
console.timeEnd('JSON.parse');

console.time('binary unserialization');
for(var i = 0; i < 100000; i++) {
    obj.test;
}
console.timeEnd('binary unserialization');
return;

obj.foo = "bar";

assert.strictEuqal(obj.foo, "bar");

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

