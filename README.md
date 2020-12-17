# gva_failed_sample
## build and run
```shell
git clone https://github.com/b04505009/gva_failed_sample
cd gva_failed_sample/demo_ir/cpp/
cmake . && make
./demo -c config/file/sample.json
```
## different json reading method
from line 89 to 106
```cpp
    // C style reading
    FILE * cfg_file = fopen(config_path, "r");
    if (!cfg_file) {
        printf("Cannot read the configuration json file!\n");
        exit(1);
    }
    json config = json::parse(cfg_file);
    fclose (cfg_file);
    // C++ style reading
    /*
    ifstream i(config_path);
    if (!i) {
        printf("Cannot read the configuration json file!\n");
        exit(1);
    }
    json config = json::parse(i);
    i.close();
    */
```
