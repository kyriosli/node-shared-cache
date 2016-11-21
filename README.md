## node-shared-cache

Interprocess shared memory cache for Node.JS

It supports auto memory-management and fast object serialization. It uses a hashmap and LRU cache internally to maintain its contents.

## Updates

  - 1.6.2
    - Add `exchange` method which can be used as atomic lock as well as `increase`
    - Add `fastGet` method which does not touch the LRU sequence
  - 1.6.1 Update `nan` requirement to 2.4.0 
  - 1.6.0 Add support for Win32 ([#7](https://github.com/kyriosli/node-shared-cache/issues/7)). Thanks to [@matthias-christen](https://github.com/matthias-christen) [@dancrumb](https://github.com/dancrumb)

## Install

You can install it with npm. Just type `npm i node-shared-cache` will do it.

You can also download and install it manually, but you need to install Node.JS and `node-gyp` first.

    git clone https://github.com/kyriosli/node-shared-cache.git
    cd node-shared-cache
    node-gyp rebuild


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
// create cache instance
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

// increase a key
cache.increase(obj, "foo");
cache.increase(obj, "foo", 3);

// exchange current key with new value, the old value is returned
cache.set(obj, "foo", 123);
cache.exchange(obj, "foo", 456); // 123
obj.foo; // 456

// release memory region
cache.release("test");

// dump current cache
var values = cache.dump(obj);
// dump current cache by key prefix
values = cache.dump(obj, "foo_");
```

### class Cache

#### constructor

```js
    function Cache(name, size, optional block_size)
```

`name` represents a file name in shared memory, `size` represents memory size in bytes to be used. `block_size` denotes the size of the unit of the memory block.

`block_size` can be any of:

  - cache.SIZE_64 (6): 64 bytes (default)
  - cache.SIZE_128 (7): 128 bytes
  - cache.SIZE_256 (8): 256 bytes
  - cache.SIZE_512 (9): 512 bytes
  - cache.SIZE_1K (10): 1KB
  - cache.SIZE_2K (11): 2KB
  - ...
  - cache.SIZE_16K (14): 16KB

Note that:

  - `size` should not be smaller than 524288 (512KB)
  - block count is 32-aligned
  - key length should not be greater than `(block_size - 32) / 2`, for example, when block size is 64 bytes, maximum key length is 16 chars.
  - key length should also not be greater than 256

So when block_size is set to default, the maximum memory size that can be used is 128M, and the maximum keys that can be stored is 2088960 (8192 blocks is used for data structure)

#### property setter

```js
set(name, value)
```

### exported methods

#### release

```js
function release(name)
```

The shared memory named `name` will be released. Throws error if shared memory is not found. Note that this method simply calls `shm_unlink` and does not check whether the memory region is really initiated by this module.

Don't call this method when the cache is still used by some process, may cause memory leak

#### clear

    function clear(instance)

Clears a cache

#### increase

```js
function increase(instance, name, optional increase_by)
```

Increase a key in the cache by an integer (default to 1). If the key is absent, or not an integer, the key will be set to `increase_by`.

#### exchange

```js
function exchange(instance, name, new_value)
```

Update a key in the cache with a new value, the old value is returned.

#### fastGet

```js
function fastGet(instance, name)
```

Get the value of a key without touching the LRU sequence. This method is usually faster than `instance[name]` because it uses
different lock mechanism to ensure shared reading across processes.

#### dump

```js
    function dump(instance, optional prefix)
```

Dump keys and values 

## Performance

Tests are run under a virtual machine with one processor: 
```sh
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
```

Block size is set to 64 and 1MB of memory is used.

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

    plain obj: 227ms
    shared cache: 492ms (1:2.17)

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

    read plain obj: 138ms
    read shared cache: 524ms (1:3.80)

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

    read plain obj with key absent: 265ms
    read shared cache with key absent: 595ms (1:2.24)

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

    enumerate plain obj: 1201ms
    enumerate shared cache: 4262ms (1:3.55)

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

    JSON.stringify: 5876ms
    binary serialization: 2523ms (2.33:1)
    JSON.parse: 2042ms
    binary unserialization: 2098ms (1:1.03)


## TODO
