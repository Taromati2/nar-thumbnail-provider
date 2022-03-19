# NAR thumbnail provider
![图片](https://user-images.githubusercontent.com/31927825/159116281-9b32ab89-2bad-433b-85b0-bda347773b77.png)  
NAR thumbnail provider for Windows Explorer  
[Download latest installer](https://github.com/Taromati2/nar-thumbnail-provider/releases/latest)  

# How do I get my nar to support getting thumbnail?
1. Compressed packages using LZMA/LZMA2 format  
   ![图片](https://user-images.githubusercontent.com/31927825/159008872-2cc2c56b-4ac0-4dd9-8c94-4ae6893f4d0f.png)
2. Create a `.nar_icon` folder in the root of the archive  
   ![图片](https://user-images.githubusercontent.com/31927825/159008338-4a804873-b08c-4996-9752-7f77dcc11906.png)
3. Place any number of ico files in  
   When there are multiple ico files, one ico will be randomly selected each time the preview is generated🔮  
   Given the thumbnail caching mechanism, users may only see one icon from start to end until the drive is cleared up🗑  

# Building Requirements
* [Visual Studio](https://visualstudio.microsoft.com/vs/community/)
* [WIX Toolset](http://wixtoolset.org/) for generating setup
