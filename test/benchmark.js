var binding = require('../index.js');

var _t, hrtime = process.hrtime;
function begin() {
    _t = hrtime();
}

function end() {
    _t = hrtime(_t);
    return (_t[0] * 1e3 + _t[1] / 1e6).toFixed(2)
}

// test plain object
var plain = {};
begin();
for(var i = 0; i < 1e6; i++) {
    plain['test' + (i & 127)] = i;
}

console.log('write plain obj 100w times: %sms', end());

// test shared cache
var obj = new binding.Cache("benchmark", 1048576);
begin();
for(var i = 0; i < 1e6; i++) {
    obj['test' + (i & 127)] = i;
}
console.log('write shared cache 100w times: %sms', end());

// test read existing key
begin();
for(var i = 0; i < 1e6; i++) {
    plain['test' + (i & 127)];
}
console.log('read plain obj 100w times: %sms', end());

begin();
for(var i = 0; i < 1e6; i++) {
    obj['test' + (i & 127)];
}
console.log('read shared cache 100w times: %sms', end());

begin();
for(var i = 0; i < 1e6; i++) {
    binding.fastGet(obj, 'test' + (i & 127));
}
console.log('fastGet 100w times: %sms', end());

begin();
for(var i = 0; i < 1e6; i++) {
    plain['oops' + (i & 127)];
}
console.log('read plain obj with key absent 100w times: %sms', end());

begin();
for(var i = 0; i < 1e6; i++) {
    obj['oops' + (i & 127)];
}
console.log('read shared cache with key absent 100w times: %sms', end());

// test enumerating keys
begin();
for(var i = 0; i < 1e5; i++) {
    Object.keys(plain);
}
console.log('enumerate plain obj 10w times: %sms', end());

begin();
for(var i = 0; i < 1e5; i++) {
    Object.keys(obj);
}
console.log('enumerate shared cache 10w times: %sms', end());

// test object serialization
var input = {env: process.env, arr: [process.env, process.env]};
begin();
for(var i = 0; i < 1e5; i++) {
    JSON.stringify(input);
}
console.log('JSON.stringify 10w times: %sms', end());

begin();
for(var i = 0; i < 1e5; i++) {
    obj.test = input;
}
console.log('binary serialization 10w times: %sms', end());

// test object unserialization
input = JSON.stringify(input);
begin();
for(var i = 0; i < 1e5; i++) {
    JSON.parse(input);
}
console.log('JSON.parse 10w times: %sms', end());

begin();
for(var i = 0; i < 1e5; i++) {
    obj.test;
}
console.log('binary unserialization 10w times: %sms', end());