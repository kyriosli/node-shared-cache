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

## TODO

  - LRU cache auto recycle when insufficient memory