这是将来打算做成简谱排版器的VMT项目。

# 目标
从纯文本排版（学习TeX）出发，逐渐加入简谱和线、简混排的支持。

# 各文档的意义
## `fileReader.c`/`.h`
文本的底层读取，包括从UTF-8纯文本读取Unicode字符。将来可能会加入对UTF-16文本的支持。

## `fontObject.c`/`.h`
对SFNT字体的处理。包括cmap表的读取和从UID得出GID（或CID）。

## `main.c`
主程序入口。

## `pageList.c`/`.h`
用于将字符组合成页的部分（暂时未完成）

## `tokenAnalysis.c`/`.h`
用于将字符Tokenize。后续会加上分析整数、浮点数和长度等的内容。