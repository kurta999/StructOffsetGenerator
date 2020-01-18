# StructOffsetGenerator
A simple structure member offset generator program

Insert your structures into input.txt and the program will automatically generate offset of members into HTML or CSV format to "out" folder with "structurename.html"

- Supports exporting doxygen style comments
```cpp
#pragma pack(1)
typedef struct{
  int8_t min_core_temp;
  int8_t max_core_temp;
  int8_t avg_core_temp;
  uint64_t channels[16];
  uint16_t remaining_time
} SecondStructure
#pragma pack()
```
![alt text](https://gtfkephost.hu/files/2020/03/18/fvoti27ff7.jpg)
