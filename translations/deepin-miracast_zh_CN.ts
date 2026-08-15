<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>Application</name>
    <message>
        <location filename="../src/main.cpp" line="22"/>
        <source>Miracast</source>
        <translation>无线投屏</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="23"/>
        <source>Mirror this computer to a Miracast wireless display.</source>
        <translation>将此计算机的屏幕镜像到 Miracast 无线显示器。</translation>
    </message>
</context>
<context>
    <name>CastEngine</name>
    <message>
        <location filename="../src/engine/castengine.cpp" line="29"/>
        <source>Idle. Scan to search for Miracast displays.</source>
        <translation>空闲。点击“扫描”以搜索 Miracast 显示器。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="119"/>
        <source>Enter the pairing PIN for %1…</source>
        <translation>请输入 %1 的配对 PIN…</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="121"/>
        <source>Confirm pairing on %1…</source>
        <translation>请在 %1 上确认配对…</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="143"/>
        <source>Stop the current session before scanning.</source>
        <translation>请先结束当前会话再扫描。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="148"/>
        <location filename="../src/engine/castengine.cpp" line="265"/>
        <source>Scanning for Miracast displays…</source>
        <translation>正在扫描 Miracast 显示器…</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="163"/>
        <source>Select a display first.</source>
        <translation>请先选择一台显示器。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="169"/>
        <source>Unknown display.</source>
        <translation>未知显示器。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="175"/>
        <source>Connecting…</source>
        <translation>正在连接…</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="180"/>
        <source>No capture backend for this session.</source>
        <translation>当前会话没有可用的屏幕采集后端。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="199"/>
        <source>Disconnected.</source>
        <translation>已断开。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="268"/>
        <source>Found %1 device(s) (%2 with WFD IEs).</source>
        <translation>发现 %1 台设备（其中 %2 台带有 WFD 信息元素）。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="282"/>
        <source>No P2P devices found. The sink must be in wireless-display / Miracast mode.</source>
        <translation>未发现 P2P 设备。请将接收端置于无线显示 / Miracast 模式。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="294"/>
        <source>Found %1 P2P device(s) with no WFD IEs. wpa_supplicant may lack CONFIG_WIFI_DISPLAY, or they are not Miracast sinks.</source>
        <translation>发现 %1 台没有 WFD 信息元素的 P2P 设备。wpa_supplicant 可能未启用 CONFIG_WIFI_DISPLAY，或它们不是 Miracast 接收端。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="298"/>
        <source>Scan finished. %1 Miracast display(s) available.</source>
        <translation>扫描完成。可用的 Miracast 显示器：%1 台。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="313"/>
        <source>Timed out forming the Wi-Fi Direct group or WFD session.</source>
        <translation>建立 Wi-Fi Direct 组或 WFD 会话超时。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="322"/>
        <source>Wi-Fi Direct group dropped.</source>
        <translation>Wi-Fi Direct 组已断开。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="332"/>
        <source>Mirroring %1.</source>
        <translation>正在镜像 %1。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="350"/>
        <source>Starting encoder (%1, %2, %3)…</source>
        <translation>正在启动编码器（%1，%2，%3）…</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="456"/>
        <source>%1 (%2×%3)%4</source>
        <translation>%1（%2×%3）%4</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="460"/>
        <source> · primary</source>
        <translation> · 主屏</translation>
    </message>
</context>
<context>
    <name>GstEncoder</name>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="37"/>
        <source> from %1</source>
        <translation> 来自 %1</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="56"/>
        <source>Missing sink IP or RTP port.</source>
        <translation>缺少接收端 IP 或 RTP 端口。</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="64"/>
        <location filename="../src/session/gstencoder.cpp" line="250"/>
        <source>no Pulse monitor, video only</source>
        <translation>没有 Pulse 监听源，仅视频</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="94"/>
        <source>No working encoder. Need a 1.24-compatible mpegtsmux or ffmpeg with libx264.</source>
        <translation>没有可用的编码器。需要与 1.24 兼容的 mpegtsmux，或带 libx264 的 ffmpeg。</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="212"/>
        <source>gst-launch-1.0 failed to start.</source>
        <translation>gst-launch-1.0 启动失败。</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="225"/>
        <source>ffmpeg not found (needed because GStreamer mpegtsmux will not load).</source>
        <translation>未找到 ffmpeg（因 GStreamer mpegtsmux 无法加载而需要它）。</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="277"/>
        <source>ffmpeg failed to start.</source>
        <translation>ffmpeg 启动失败。</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="317"/>
        <source>encoder exited with code %1</source>
        <translation>编码器退出，返回码 %1</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="37"/>
        <source>Miracast</source>
        <translation>无线投屏</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="75"/>
        <source>Monitor</source>
        <translation>显示器</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="87"/>
        <source>Include system audio</source>
        <translation>包含系统音频</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="101"/>
        <source>Scan</source>
        <translation>扫描</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="102"/>
        <location filename="../src/ui/mainwindow.cpp" line="285"/>
        <source>Connect</source>
        <translation>连接</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="103"/>
        <source>Disconnect</source>
        <translation>断开</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="122"/>
        <source>Display server: unknown</source>
        <translation>显示服务器：未知</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="125"/>
        <source>Display server: X11</source>
        <translation>显示服务器：X11</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="128"/>
        <source>Display server: Wayland</source>
        <translation>显示服务器：Wayland</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="171"/>
        <source>No Miracast displays found</source>
        <translation>未找到 Miracast 显示器</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="190"/>
        <source>P2P · no WFD IEs</source>
        <translation>P2P · 无 WFD 信息元素</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="191"/>
        <source>%1 · no WFD IEs</source>
        <translation>%1 · 无 WFD 信息元素</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="272"/>
        <source>the display</source>
        <translation>显示器</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="279"/>
        <source>Enter pairing PIN</source>
        <translation>输入配对 PIN</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="280"/>
        <source>Enter the PIN shown on %1.</source>
        <translation>请输入 %1 上显示的 PIN。</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="282"/>
        <source>4 or 8 digit PIN</source>
        <translation>4 位或 8 位 PIN</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="286"/>
        <location filename="../src/ui/mainwindow.cpp" line="310"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="301"/>
        <source>Enter a 4- or 8-digit PIN</source>
        <translation>请输入 4 位或 8 位 PIN</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="308"/>
        <source>Confirm pairing</source>
        <translation>确认配对</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="309"/>
        <source>Confirm the pairing request on %1.</source>
        <translation>请在 %1 上确认配对请求。</translation>
    </message>
</context>
<context>
    <name>P2PDiscovery</name>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="66"/>
        <source>System D-Bus is not available.</source>
        <translation>系统 D-Bus 不可用。</translation>
    </message>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="72"/>
        <source>Wi-Fi is off. Turn it on to search for Miracast displays.</source>
        <translation>Wi-Fi 已关闭。请打开 Wi-Fi 后再搜索 Miracast 显示器。</translation>
    </message>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="79"/>
        <source>No Wi-Fi P2P adapter found. Enable Wi-Fi and use NetworkManager with wpa_supplicant P2P (not iwd).</source>
        <translation>未找到 Wi-Fi P2P 适配器。请启用 Wi-Fi，并使用带 wpa_supplicant P2P 的 NetworkManager（不要使用 iwd）。</translation>
    </message>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="116"/>
        <source>Could not start Wi-Fi Direct find.</source>
        <translation>无法开始 Wi-Fi Direct 搜索。</translation>
    </message>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="124"/>
        <source>Scanning for Miracast displays…</source>
        <translation>正在扫描 Miracast 显示器…</translation>
    </message>
</context>
<context>
    <name>P2PSession</name>
    <message>
        <location filename="../src/session/p2psession.cpp" line="67"/>
        <source>Sink is missing P2P address or device path.</source>
        <translation>接收端缺少 P2P 地址或设备路径。</translation>
    </message>
    <message>
        <location filename="../src/session/p2psession.cpp" line="116"/>
        <source>Forming Wi-Fi Direct group…</source>
        <translation>正在建立 Wi-Fi Direct 组…</translation>
    </message>
</context>
<context>
    <name>PortalCapture</name>
    <message>
        <location filename="../src/capture/portalcapture.cpp" line="12"/>
        <source>Screen capture is unavailable on this session. xdg-desktop-portal ScreenCast is not available.</source>
        <translation>当前会话无法采集屏幕。xdg-desktop-portal ScreenCast 不可用。</translation>
    </message>
</context>
<context>
    <name>WfdAudioMode</name>
    <message>
        <location filename="../src/session/wfdaudiomode.cpp" line="37"/>
        <source>none</source>
        <translation>无</translation>
    </message>
    <message>
        <location filename="../src/session/wfdaudiomode.cpp" line="38"/>
        <source>AAC %1 kHz</source>
        <translation>AAC %1 kHz</translation>
    </message>
</context>
<context>
    <name>WfdServer</name>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="266"/>
        <source>Cannot listen on RTSP port %1: %2</source>
        <translation>无法在 RTSP 端口 %1 上监听：%2</translation>
    </message>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="272"/>
        <source>Waiting for the display on port %1…</source>
        <translation>正在端口 %1 上等待显示器…</translation>
    </message>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="306"/>
        <source>Display connected, starting WFD handshake…</source>
        <translation>显示器已连接，正在开始 WFD 握手…</translation>
    </message>
</context>
<context>
    <name>WfdSession</name>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="111"/>
        <source>WFD OPTIONS, querying sink…</source>
        <translation>WFD OPTIONS，正在查询接收端…</translation>
    </message>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="139"/>
        <source>WFD SETUP, RTP port %1</source>
        <translation>WFD SETUP，RTP 端口 %1</translation>
    </message>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="211"/>
        <source>WFD GET_PARAMETER…</source>
        <translation>WFD GET_PARAMETER…</translation>
    </message>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="227"/>
        <source>WFD SET_PARAMETER %1, %2…</source>
        <translation>WFD SET_PARAMETER %1，%2…</translation>
    </message>
</context>
<context>
    <name>X11Capture</name>
    <message>
        <location filename="../src/capture/x11capture.cpp" line="13"/>
        <source>DISPLAY is not set; cannot grab the X11 screen.</source>
        <translation>未设置 DISPLAY，无法采集 X11 屏幕。</translation>
    </message>
    <message>
        <location filename="../src/capture/x11capture.cpp" line="18"/>
        <source>No monitor selected.</source>
        <translation>未选择显示器。</translation>
    </message>
</context>
</TS>
