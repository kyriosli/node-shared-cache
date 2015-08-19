{
  "targets": [
    {
      "target_name": "binding",
      "sources": [
        "binding.cc",
        "memcache.cc",
        "bson.cc"
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
