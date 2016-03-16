var binding = require('../index.js');
try {
	binding.release('counter');
} catch(e) {}

var assert = require('assert');

var obj = new binding.Cache("counter", 1048576);

assert.strictEqual(binding.increase(obj, 'foo'), 1, 'key absent');
assert.strictEqual(binding.increase(obj, 'foo'), 2);
assert.strictEqual(binding.increase(obj, 'foo'), 3);
assert.strictEqual(binding.increase(obj, 'foo', 7), 10);
assert.strictEqual(obj.foo, 10);
obj.foo = 1234;
assert.strictEqual(binding.increase(obj, 'foo', 3), 1237);
obj.foo = "wtf";
assert.strictEqual(binding.increase(obj, 'foo', 3), 3);

assert.deepEqual(binding.dump(obj), {foo: 3});
