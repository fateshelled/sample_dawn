### dependencies
```bash
 sudo apt install libx11-dev  libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libx11-xcb-dev
 ```

### build
```bash
# Git clone
git clone 
mkdir build
cd build
cmake -G Ninja ..
ninja -j 4
```

### run
```bash
cd build 

./device_query

./ply_viewer ../data/source.ply
```