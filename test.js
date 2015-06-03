
var binding = require('./build/Release/binding.node');

var obj = new binding.Cache("test", 4194304);

obj.foo = "bar";

console.log(obj.foo);

delete obj.foo;
'foo' in obj;
console.log(obj.foo);
