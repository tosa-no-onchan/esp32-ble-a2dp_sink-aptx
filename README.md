# esp32-ble-a2dp_sink-aptx

This program is Blue tooth music device aptx.  

    Borad: ESP32  
    Build Frame work: esp-idf  
    Build tool: idf v3.3.1 ?  
    Environment: Windows 10  

Base program is esp-idf/examples/bluetooth/a2dp_sink and other.  

Special Thanks libopenaptx  
  
[https://github.com/pali/libopenaptx](https://github.com/pali/libopenaptx)

### Set up build tool

Download ESPRESSIF Windows all-in-on toolchan&MSYS2(Zip)  
  
     https://dl.espressif.com/dl/esp32_win32_msys2_environment_and_toolchain-20181001.zip ?  
[https://docs.espressif.com/projects/esp-idf/en/v3.3.1/get-started/windows-setup.html#toolchain-setup](https://docs.espressif.com/projects/esp-idf/en/v3.3.1/get-started/windows-setup.html#toolchain-setup)  
  
and unzip
  
### Download this program

     >git clone https://github.com/tosa-no-onchan/esp32-ble-a2dp_sink-aptx

     cd path-to-clone-dir/esp32-ble-a2dp_sink-aptx/esp-idf/  
     COPY /B git-files.zip.001+git-files.zip.002+...+git-files.zip.006 git-files.zip
     unzip git-files.zip to esp-idf/ dir  

### Configure before build  

    * exec msys32-esp\mingw32.exe
    $IDF_PATH=/path-to-clone-dir/esp32-ble-a2dp_sink-aptx/esp-idf
    $python2.7 -m pip install --user -r $IDF_PATH/requirements.txt
    
    $cd /path-to-clone-dir/esp32-ble-a2dp_sink-aptx/a2dp_sink
    $make menuconfig
  
         
### Build and Flash  

    $make  
    $make flash  
    
### Caution  
This programs are incomplete yet.    
Its sound contains many white noise.   
I hope,someone improve this noise.   

### My making program history  
  
Look [blog](http://www.netosa.com/blog/cat2/esp32-esp-idf/bluetooth-a2dp/aptx-aptx-hd/)  




