# NAR thumbnail provider
![downloads](https://img.shields.io/github/downloads/Taromati2/nar-thumbnail-provider/total)
![preview](https://user-images.githubusercontent.com/31927825/159116281-9b32ab89-2bad-433b-85b0-bda347773b77.png)  
NAR thumbnail provider for Windows Explorer  
[Download latest installer](https://github.com/Taromati2/nar-thumbnail-provider/releases/latest)  

# How do I get my nar to support getting thumbnail?
1. Create a `.nar_icon` folder in the root of the archive  
   ![apng](https://user-images.githubusercontent.com/31927825/159008338-4a804873-b08c-4996-9752-7f77dcc11906.png)
2. Place any number of ico files in  
   When there are multiple ico files, one ico will be randomly selected each time the preview is generated🔮  
   Given the thumbnail caching mechanism, users may only see one icon from start to end until the drive is cleared up🗑  

# Building Requirements
* [Visual Studio](https://visualstudio.microsoft.com/vs/community/)
* [WIX Toolset](http://wixtoolset.org/) for generating setup
