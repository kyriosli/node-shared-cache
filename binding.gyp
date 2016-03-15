{
  "targets": [
    {
      "target_name": "binding",
      "sources": [
        "src/binding.cc",
        "src/memcache.cc",
        "src/bson.cc"
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      "conditions": [
        [
          "OS==\"linux\"",
          {
            "link_settings": {
              "libraries": [
                "-lrt"
              ]
            }
          }
        ]
      ]
    }
  ]
}
