## node-shared-cache

Interprocess shared memory cache for Node.JS

It supports auto memory-management and fast object serialization. It uses a hashmap and LRU cache internally to maintain its contents.

## Install

Install `node-gyp` first if you do not have it installed:

    sudo npm install node-gyp -g

Then

    npm install kyriosli/node-shared-cache


## Terms of Use

This software (source code and its binary builds) is absolutely copy free and any download or modification is permitted except for unprohibited
commercial use.

But due to the complexity of this software, any bugs or runtime exceptions could happen when programs which includeed it run into an unexpected
situation, which in most cases should be harmless but also have the chance to cause:

  - program crash
  - system down
  - software damage
  - hardware damage

which would lead to data corruption or even economic losses.

So when you are using this software, DO

  - check the data
  - double check the data
  - avoid undefined behavior to happen

To avoid data crupption, we use a read-write lock to ensure that data modification is exclusive. But when a program is writting data when something
bad, for example, a SIGKILL, happens that crashes the program before the write operation is complete and lock is released, other processes may not be
able to enter the exclusive region again. I do not use an auto recovery lock such as `flock`, which will automatically release when process exits, just
in case that wrong data is returned when performing a reading operation, or even, causing a segment fault.

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

    function Cache(name, size, optional block_size)

`name` represents a file name in shared memory, `size` represents memory size in bytes to be used. `block_size` denotes the size of the unit of the memory block.

`block_size` can be any of:

  - cache.SIZE_64 (6): 64 bytes (default)
  - cache.SIZE_128 (7): 128 bytes
  - cache.SIZE_256 (8): 256 bytes
  - cache.SIZE_512 (9): 512 bytes
  - cache.1K (10): 1KB
  - cache.2K (11): 2KB

Note that:

  - `size` should not be smaller than 524288 (512KB)
  - total block count (`size` / 1 << `block_size`) should not be larger than 2097152
  - block count is 32-aligned
  - key length should not be larger than `(block_size - 32) >> 1`, for example, when block size is 64 bytes, maximum key length is 16 chars.

So when block_size is set to default, the maximum memory size that can be used is 128M, and the maximum keys that can be stored is 2088960 (8192 blocks is used for data structure)

#### property setter

    set(name, value)

Note that:

  - the length of name should not be longer than 256 characters (limited by internal design)

## Performance

Tests are run under a virtual machine with one processor: 

    $ node -v
    v0.12.4
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

    plain obj: 229ms
    shared cache: 588ms

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

    read plain obj: 135ms
    read shared cache: 639ms

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

    read plain obj with key absent: 254ms
    read shared cache with key absent: 538ms

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

    enumerate plain obj: 1218ms
    enumerate shared cache: 4294ms

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

    JSON.stringify: 6183ms
    binary serialization: 2633ms
    JSON.parse: 2083ms
    binary unserialization: 2225ms


## TODO

  - add dead-lock auto recovery when data is inconsistent
