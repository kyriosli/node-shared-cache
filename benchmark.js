var binding = require('./index.js');

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