# Bilibili 弹幕工具集

**警告： 该项目暂未准备好用于生产。API、文档、用法的更改恕不另行通知。**

## XML转ASS工具

[![Build status][github-action-build-image]][github-action-build-url]    [![license][license-image]][license-url]

[github-action-build-image]: https://github.com/windowsair/bilibili_danmuku/actions/workflows/build-binary.yml/badge.svg
[github-action-build-url]: https://github.com/windowsair/bilibili_danmuku/actions/workflows/build-binary.yml


[license-image]: https://img.shields.io/badge/license-GPLv3-green.svg
[license-url]: https://github.com/windowsair/corsacOTA/LICENSE


将录制好的原始XML格式弹幕转换为ASS样式。


----

## 用法


基本使用
```bash
./xml2ass <xml_file1> <xml_file2> ...

# 可以这样做--->
$ ./xml2ass 1.xml 2.xml 3.xml 
```

或者输入一个目录，将转换该目录下同级的所有xml

```bash
$ ./xml2ass ./xml_path
```

混合输入也是可行的

```bash
$ ./xml2ass ./xml_path ./1.xml
```

### 自定义配置

在首次运行时，会自动生成默认的配置文件`config.json`
您可以按照文件中的提示修改配置

```json
{
	"video_width": 1920,
	"#video_width": "视频宽度",

	"video_height": 1080,
	"#video_height": "视频高度",

	"font_family": "微软雅黑",
	"#font_family": "采用的字体集",
  
	"font_scale": 1.6,
	"#font_scale": "字体缩放倍数，为1.0时保持原始大小",

	"font_alpha": 0.75,
	"#font_alpha": "字体透明度,取值为0~1.0,为0时完全透明",
  
	"font_bold": true,
	"#font_bold": "是否设置字体加粗,true加粗,false不加粗",

	"danmaku_show_range": 0.45,
	"#danmaku_show_range": "弹幕在屏幕上的显示范围，取值为0~1.0，为1时全屏显示",

	"danmaku_move_time": 15,
	"#danmaku_move_time": "滚动弹幕的停留时间(以秒计)，设置为-1表示忽略滚动弹幕",

	"danmaku_pos_time": 5,
	"#danmaku_pos_time": "固定弹幕的停留时间(以秒计)，设置为-1表示忽略固定弹幕"
}
```


## 构建与编译

您可以自行编译项目，或者直接使用预先构建好的二进制文件。

编译需要用到cmake依赖，以Linux为例，典型的构建流程如下：
```bash
$ mkdir build && cd build
$ cmake ..
$ make
```


## 预构建二进制文件下载

目前已有amd64体系架构的Windows, Linux, MacOS的预编译二进制文件。 ARM等体系架构的二进制文件需要您自行编译。
这些预构建二进制文件可以在这里下载到： [预编译文件](https://github.com/windowsair/bilibili_danmaku/actions/workflows/build-binary.yml)


有关如何下载，请参考：[github action帮助](https://docs.github.com/cn/actions/managing-workflow-runs/downloading-workflow-artifacts)


# 第三方项目

本项目直接或间接使用到了这些项目，感谢他们。

某些项目可能有改动，改动后的项目遵循其原有的许可证。

- `pugixml` MIT License
- `fmtlib` MIT License
- `rapidjson` MIT License
- `simdutf` MIT License
- `re2` BSD 3-Clause License
- `IXWebSocket` BSD 3-Clause License
- `openssl` Apache License 2.0
- `readwritequeue` BSD License
- `libdeflate` MIT License

# Credit

- [海面烧烧炮](https://space.bilibili.com/2437955) 感谢他提供的想法，没有他就没有本项目。

