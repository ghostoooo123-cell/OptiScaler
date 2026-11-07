# 非官方简体中文界面

这是 OptiScaler **v0.9.4** 的非官方界面翻译，只存在于本 fork。它与 OptiScaler 团队无关，也不替代上游发行版。

本改动只翻译游戏内 ImGui 叠加层的标签、按钮和提示，并补上简体中文字形。它不修改超分、帧生成、挂钩、伪装或任何安全相关逻辑，也不包含针对某一款游戏的部署步骤。

## 语言开关

叠加层底部有 **Language / 语言**。

- `en`：英文。控件 ID 和显示文字与 v0.9.4 相同。
- `zh-CN`：简体中文。没有译文的字符串仍显示英文。窗口标题会加上 ` [非官方简中]`。

选择会立刻改变界面语言。写入 `OptiScaler.ini` 需要按 **Save Settings / 保存设置**。

```ini
[Menu]
Language=zh-CN
LoadCjkFont=true
```

`Language` 也接受 `zh`、`zh_cn`、`chinese`、`cn`、`chs`、`zh-hans`。默认 `auto`，也就是英文。`LoadCjkFont` 默认开启。字体在初始化时合并进图集，因此来回切换语言不需要重建字体纹理。

## 字体

优先使用内嵌的 Noto Sans SC 子集，名称是 **OptiScaler UI SC**，不是 Adobe 的保留名称 Source。版权与许可：

- (c) 2014-2021 Adobe, with Reserved Font Name "Source".
- SIL Open Font License 1.1: https://scripts.sil.org/OFL

子集只包含界面译文里用到的字形。若内嵌字体不可用，按下面顺序查找：

1. DLL 旁边的 `OptiScalerZhCN.ttf`
2. `C:\Windows\Fonts\msyh.ttc`
3. `C:\Windows\Fonts\msyh.ttf`
4. `C:\Windows\Fonts\simhei.ttf`
5. `C:\Windows\Fonts\simsun.ttc`

`LoadCjkFont=false` 会跳过合并。这时中文会显示为缺字。

## 保持不变的内容

技术缩写保持英文，例如 DLSS、DLSSG、FSR、XeSS、XeFG、RCAS、FG、FPS、HDR、DRS、Reflex、AntiLag 2、LatencyFlex、XeLL。ImGui 控件 ID 在英文模式下与上游相同；中文模式使用 `译文###英文`，ID 载荷仍是原来的英文。printf 占位符的顺序和类型不变。配置里已有的键不会改含义。

## Windows 构建

这个仓库在 Linux 环境里不能产出 `OptiScaler.dll`。需要在 Windows 上用 Visual Studio 2022 构建。

1. 安装 Visual Studio 2022，工作负载选择“使用 C++ 的桌面开发”。
2. 工具集使用 MSVC v143，Windows SDK 使用 10.0。
3. 克隆本 fork，并拉取全部子模块：`git submodule update --init --recursive`
4. 检出带有本改动的分支，基线是 tag/branch `v0.9.4`，不要用上游最新 master。
5. 用 Visual Studio 2022 打开仓库根目录的 `OptiScaler.sln`。
6. 配置选 **Release**，平台选 **x64**。
7. 生成项目 **OptiScaler**（动态库，`OptiScaler.vcxproj`）。
8. 产物一般是解决方案目录下的 `x64\Release\OptiScaler.dll`。Debug 的中间目录在 `OptiScaler\x64\Debug\`。
9. 把 `OptiScaler.dll` 和原来的 `OptiScaler.ini` 放在一起。不要把本 DLL 复制进未确认的游戏目录；本说明不提供任何游戏安装或绕过保护的步骤。

本地化源文件使用 `/utf-8`，并且不使用预编译头：

- `OptiScaler/menu/locale/ui_locale.cpp`
- `OptiScaler/menu/locale/ui_locale_font.cpp`

译文表在 `tools/zh_cn_map.json`。修改后在仓库根目录运行：

```bash
python3 tools/gen_zh_cn_catalog.py --report zh_cn_coverage.txt
```

这会重写 `OptiScaler/menu/locale/zh_cn_catalog.inc` 和 `OptiScaler/menu/locale/zh_cn_font_subset.h`。生成字体需要本机的 Noto Sans SC 可变字体（`/tmp/fonts/NotoSansSC.ttf`）和 fontTools。没有字体文件时，脚本会保留已有查找表并跳过字体；提交前需要确认子集头不是占位的空数组。

不依赖 Windows 的查找表检查：

```bash
g++ -std=c++20 -Wall -Wextra -I OptiScaler -I OptiScaler/menu/locale -o /tmp/ui_locale_selftest tools/ui_locale_selftest.cpp OptiScaler/menu/locale/ui_locale.cpp
/tmp/ui_locale_selftest
```
