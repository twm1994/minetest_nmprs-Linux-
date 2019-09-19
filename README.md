# Minetest NMPRS (Linux)

Based on this [repository](https://github.com/twm1994/minetest_nmprs).  
 
Tested on ***Pop!\_OS 18.04 LTS***.

## Dependencies and tools
The following libraries can be installed with **apt**
 - libirrlicht-dev
 - libjthread-dev

For **jsoncpp**, my setup is as followed
 - download the [source files](https://github.com/open-source-parsers/jsoncpp) 
 - run **amalgamate.py** at the root directory of **jsoncpp** folder
 - copy the amalgamared source to **$HOME/jsoncpp**, which has the following structure
```bash
.
├── json
│   ├── json-forwards.h
│   └── json.h
└── jsoncpp.cpp
```

## How to build
I followed the instructions in the [original repository](https://github.com/celeron55/minetest_nmpr).

>Building:
```
$ make
```

>Running:
```
$ cd bin  # "bin" is required to be the working directory so that data is located at "../data"
$ ./test
```

## License

### To be updated
