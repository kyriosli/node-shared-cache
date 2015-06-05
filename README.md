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

When setting property 100w times

```js
var obj = new binding.Cache("test", 557056);

// test simple obj
var plain = {};
console.time('plain obj');
for(var i = 0; i < 1000000; i++) {
    plain['test' + (i & 127)] = i;
}

console.timeEnd('plain obj');

// test shared cache
console.time('shared cache');
for(var i = 0; i < 1000000; i++) {
    obj['test' + (i & 127)] = i;
}
console.timeEnd('shared cache');
```

The result is:

    $ node test
    plain obj: 241ms
    shared cache: 466ms