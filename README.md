## node-shared-cache

Interprocess shared memory cache for Node.JS

It supports auto memory-management and fast object serialization. It uses a hashmap and LRU cache internally to maintain its contents.

## usage

```js
var cache = require('node-shared-cache');
var obj = new cache.Cache("test", 557056);
// setting property
obj.foo = "bar";
// getting property
console.log(obj.foo);
// enumerating properties
for(var k in obj);
Object.keys(obj);
// deleting property
delete obj.foo;
// writing objects is also supported
obj.foo = {'foo': 'bar'};
// but original object reference is not saved
var test = obj.foo = {'foo': 'bar'};
test === obj.foo; // false
// circular reference is supported.
test.self = test;
obj.foo = test;
// and saved result is also circular
test = obj.foo;
test.self === test; // true
```

### class Cache

#### constructor

    function Cache(name, size)

`name` represents a file name in shared memory, `size` represents memory size in bytes to be used. Note that:

  - `size` should not be smaller than 557056 (544KB)
  - `size` should not be larger than 2147483647 (2GB)
  - `size` is 32KB aligned

#### property setter

    set(name, value)

Note that:

  - the length of name should not be longer than 256 characters (limited by internal design)

## Performance

Tests are run under a virtual machine with one processor: 

    $ cat /proc/cpuinfo
    processor   : 0
    vendor_id   : GenuineIntel
    cpu family  : 6
    model       : 45
    model name  : Intel(R) Xeon(R) CPU E5-2630 0 @ 2.30GHz
    stepping    : 7
    microcode   : 0x70d
    cpu MHz     : 2300.090
    cache size  : 15360 KB    
    ...

### Setting property

When setting property 100w times:

```js
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
```

The result is:

    plain obj: 234ms
    shared cache: 448ms

### Getting property

When trying to read existing key:

```js
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
```

The result is:

    read plain obj: 127ms
    read shared cache: 499ms

When trying to read keys that are not existed:

```js
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
```

The result is:

    read plain obj with key absent: 262ms
    read shared cache with key absent: 353ms

### Enumerating properties

When enumerating all the keys:

```js
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
```

The result is:

    enumerate plain obj: 657ms
    enumerate shared cache: 2698ms

Warn: Because the shared memory can be modified at any time even the current Node.js
process is running, depending on keys enumeration result to determine whether a key
is cached is unwise. On the other hand, it takes so long a time to build strings from
memory slice, as well as putting them into an array, so DO NOT USE IT unless you know
that what you are doing.

### Object serialization

We choose a c-style binary serialization method rather than `JSON.stringify`, in two
concepts:

  - Performance serializing and unserializing
  - Support for circular reference

Tests code list:

```js
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
```

The result is:

    JSON.stringify: 6846ms
    binary serialization: 2125ms
    JSON.parse: 1557ms
    binary unserialization: 1261ms

