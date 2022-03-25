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

```bash
./xml2ass <xml_file1> <xml_file2> ...

# 可以这样做->
$ ./xml2ass 1.xml 2.xml 3.xml 
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

目前已有amd64体系架构的Windows, Linux, MacOS的预编译二进制文件。 ARM等体系架构的二进制文件需要您自行进行编译。
这些预构建二进制文件可以在这里下载到： [预编译文件](https://github.com/windowsair/bilibili_danmuku/actions/workflows/build-binary.yml)


有关如何下载，请参考：[github action帮助](https://docs.github.com/cn/actions/managing-workflow-runs/downloading-workflow-artifacts)


# 第三方项目

本项目直接或间接使用到了这些项目，感谢他们。

- `pugixml` MIT license
- `scnlib` Apache license
- `fmtlib` MIT license
- `utfcpp` BSL-1.0 License


# Credit

- [海面烧烧炮](https://space.bilibili.com/2437955)