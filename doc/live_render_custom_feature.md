<h1>live_render 自定义特性参考文档</h1>


- [一般规则](#一般规则)
  - [输入流配置](#输入流配置)
  - [滤镜（filter）配置](#滤镜filter配置)
  - [调试自定义filter和输入流](#调试自定义filter和输入流)
  - [注意事项](#注意事项)
- [案例](#案例)
  - [调整输出视频尺寸（伪4K生成）](#调整输出视频尺寸伪4k生成)


live_render可以借助FFmpeg中libavfilter提供的复杂滤镜(complex filter)功能，完成视频后期的简单处理。这些处理包括添加水印、调整视频比例等。有关FFmpeg filter的更多信息，请见https://ffmpeg.org/ffmpeg-filters.html


本文将介绍如何使用这一特性，并附上一些案例。如果您想要实现某个本文中未提到的功能，也欢迎打开一个[issue](https://github.com/windowsair/bilibili_danmaku/issues)告诉我们。

# 一般规则

目前live_render支持多个输入流和单个输出流，不支持音频流。

对于额外的输入流(input stream)和自定义的filter，分别修改配置文件中的`extra_input_stream_info`和`extra_filter_info`。

## 输入流配置

live_render中的额外输入流遵循ffmpeg的一般格式，即:
```
{[input_file_options] -i input_url}
```

其中，`[input_file_options]`为可选的输入文件选项，`input_url`为输入流的uri，花括号代表可以有多个输入流。



下面是一个简单例子：指定了两个额外的输入流，分别是一张图片和一段视频。
```json
"extra_input_stream_info": "-i C:/1.png -i C:/2.mp4"
```

在`extra_input_stream_info`中输入的输入流序号从`2`开始递增。

> 对于上面的例子,图片对应的label为`[2:v]`，视频对应的label为`[3:v]`。live_render内部会占用两个流，分别为`[0:v]`和`[1:v]`


## 滤镜（filter）配置

live_render中采用FFmpeg的complex filter来完成一些高级功能。

> 您可能使用过FFmpeg的"-vf"选项，实际上这就是单个流的"complex filter"。

live_render所采用的complex filter如下图所示：

```bash
 _____________
| live_render |
|  internal   |
|   input 0   |\
|_____________| \
                 \    _________
                  \  |         |
 _____________     \ | complex |      __________
| live_render |      |         |     |          |
|  internal   |----> | filter  |---> | output 0 |
|   input 1   |      |         |     |__________|
|_____________|    / |  graph  |
                  /  |         |
                 /   |         |
                /    |_________|
 _____________ /
| User        |
|  input 2    |
|_____________|
```

其中，live_render内部占用两个输入流（他们分别是原始直播视频以及弹幕视频），然后与用户自定义输入的流经过complex filter处理后输出最终的视频。

**live_render处理完弹幕的视频流的label为`[v0]`，最终输出的视频流应指定为`[v]`。**

将上图转换为对应的label，可以得到下图所示的关系：

```bash
                      _________
                     |         |
 _____________       | complex |      __________
|             |      |         |     |          |
|    [v0]     |----> | filter  |---> |    [v]   |
|_____________|      |         |     |__________|
                   / |  graph  |
                  /  |         |
                 /   |         |
                /    |_________|
______________ /
|             |
|    [2:v]    |
|_____________|
```

----

下面，以添加水印为例，介绍如何进行自定义complex filter。

```json
"extra_input_stream_info": "-i C:/1.png",
"extra_filter_info": "[v0][2:v]overlay[v]"
```

在这个例子中，我们额外添加了一个输入流，它是一张图片，路径为`C:/1.png`。这个输入流为用户自定义输入流，输入序号由`2`开始递增。该图片对应的label为`[2:v]`。

我们使用`overlay`滤镜进行处理，将图片`[2:v]`覆盖到live_render的原始视频`[v0]`上。最后，我们显式指定了overlay滤镜的输出为`[v]`。


## 调试自定义filter和输入流

live_render会输出原始的FFmpeg命令，此外FFmpeg的相关信息均会打印出来。可以借助程序输出的日志来进行调试。


## 注意事项

1) 在输入流指定路径(uri)时，不能含有空格。

----

2) 当设置`extra_input_stream_info`和`extra_filter_info`遇到特殊字符时，需要进行转义处理，主要用作json的转义。

    举例来说，当要处理`C:\1.jpg`时，由于其中有特殊字符`\`，需要进行转义，变为`C:\\1.jpg`。

----

3) 所有以`lr_`开头的label保留为live_render内部使用，不要在filter中使用他们。

----

4) live_render输出的原始视频流`[v0]`位于system memory中，如果后续的某些filter操作需要借助 硬件加速功能，需要使用hwupload等选项上传到hardware surface。




# 案例


## 调整输出视频尺寸（伪4K生成）

动机： 将1080P的原视频转换为4K大小的视频输出。


下面是使用CPU对视频大小进行调整的通用方法：
```json
"extra_input_stream_info": "",

"extra_filter_info": "[v0]scale={1}:flags={2}[v]",
```

- `{1}`输出视频的尺寸，按照`w:h`的格式进行输入。
- `{2}`为放缩过程中的额外选项，例如指定缩放算法：
    - `bicubic` 双三次插值(CPU/Nvidia)
    - `bilinear` 双线性插值(CPU/Nvidia)
    - `neighbor` 最近邻插值(CPU/Nvidia)
    - `lanczos` lanczos算法(CPU/Nvidia)
    - `bicublin` 亮度分量双三次缩放插值(CPU)
    - `fast_bilinear` 快速双线性插值(CPU)
    - `area` 平均面积插值(CPU)
    - `gauss` 高斯插值(CPU)
    - `sinc` sinc插值(CPU)
    - `spline` (natural bicubic spline算法)(CPU)
    - `experimental` 实验性算法(CPU)


若期望的输出视频大小为`2564:1442`，采用`bicubic`算法进行放大，则对应的配置为：
```json
"extra_input_stream_info": "",

"extra_filter_info": "[v0]scale=2564:1442:flags=bicubic[v]",
```

----

当采用Nvidia GPU时，可以借助Nvidia的硬件加速功能：
```json
"extra_input_stream_info": "",

"extra_filter_info": "[v0]hwupload_cuda[v1];[v1]scale_cuda={1}:interp_algo={2}[v]",
```

- `{1}`输出视频的尺寸，按照`w:h`的格式进行输入。
- `{2}`为放缩过程中的额外选项，例如指定缩放算法：
    - `bicubic` 双三次插值(CPU/Nvidia)
    - `bilinear` 双线性插值(CPU/Nvidia)
    - `neighbor` 最近邻插值(CPU/Nvidia)
    - `lanczos` lanczos算法(CPU/Nvidia)


若期望的输出视频大小为`2564:1442`，采用`bicubic`算法进行放大，则对应的配置为：
```json
"extra_input_stream_info": "",

"extra_filter_info": "[v0]hwupload_cuda[v1];[v1]scale_cuda=2564:1442:interp_algo=bicubic[v]",
```
