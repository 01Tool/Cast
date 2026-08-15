<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_TW">
<context>
    <name>Application</name>
    <message>
        <location filename="../src/main.cpp" line="22"/>
        <source>Miracast</source>
        <translation>無線投屏</translation>
    </message>
    <message>
        <location filename="../src/main.cpp" line="23"/>
        <source>Mirror this computer to a Miracast wireless display.</source>
        <translation>將此電腦的畫面鏡像到 Miracast 無線顯示器。</translation>
    </message>
</context>
<context>
    <name>CastEngine</name>
    <message>
        <location filename="../src/engine/castengine.cpp" line="29"/>
        <source>Idle. Scan to search for Miracast displays.</source>
        <translation>閒置。按「掃描」以搜尋 Miracast 顯示器。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="119"/>
        <source>Enter the pairing PIN for %1…</source>
        <translation>請輸入 %1 的配對 PIN…</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="121"/>
        <source>Confirm pairing on %1…</source>
        <translation>請在 %1 上確認配對…</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="143"/>
        <source>Stop the current session before scanning.</source>
        <translation>請先結束目前工作階段再掃描。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="148"/>
        <location filename="../src/engine/castengine.cpp" line="265"/>
        <source>Scanning for Miracast displays…</source>
        <translation>正在掃描 Miracast 顯示器…</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="163"/>
        <source>Select a display first.</source>
        <translation>請先選擇一台顯示器。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="169"/>
        <source>Unknown display.</source>
        <translation>未知顯示器。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="175"/>
        <source>Connecting…</source>
        <translation>正在連線…</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="180"/>
        <source>No capture backend for this session.</source>
        <translation>目前工作階段沒有可用的畫面擷取後端。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="199"/>
        <source>Disconnected.</source>
        <translation>已中斷連線。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="268"/>
        <source>Found %1 device(s) (%2 with WFD IEs).</source>
        <translation>找到 %1 台裝置（其中 %2 台帶有 WFD 資訊元素）。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="282"/>
        <source>No P2P devices found. The sink must be in wireless-display / Miracast mode.</source>
        <translation>未找到 P2P 裝置。請將接收端設為無線顯示 / Miracast 模式。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="294"/>
        <source>Found %1 P2P device(s) with no WFD IEs. wpa_supplicant may lack CONFIG_WIFI_DISPLAY, or they are not Miracast sinks.</source>
        <translation>找到 %1 台沒有 WFD 資訊元素的 P2P 裝置。wpa_supplicant 可能未啟用 CONFIG_WIFI_DISPLAY，或它們不是 Miracast 接收端。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="298"/>
        <source>Scan finished. %1 Miracast display(s) available.</source>
        <translation>掃描完成。可用的 Miracast 顯示器：%1 台。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="313"/>
        <source>Timed out forming the Wi-Fi Direct group or WFD session.</source>
        <translation>建立 Wi-Fi Direct 群組或 WFD 工作階段逾時。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="322"/>
        <source>Wi-Fi Direct group dropped.</source>
        <translation>Wi-Fi Direct 群組已中斷。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="332"/>
        <source>Mirroring %1.</source>
        <translation>正在鏡像 %1。</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="350"/>
        <source>Starting encoder (%1, %2, %3)…</source>
        <translation>正在啟動編碼器（%1，%2，%3）…</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="456"/>
        <source>%1 (%2×%3)%4</source>
        <translation>%1（%2×%3）%4</translation>
    </message>
    <message>
        <location filename="../src/engine/castengine.cpp" line="460"/>
        <source> · primary</source>
        <translation> · 主要</translation>
    </message>
</context>
<context>
    <name>GstEncoder</name>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="37"/>
        <source> from %1</source>
        <translation> 來自 %1</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="56"/>
        <source>Missing sink IP or RTP port.</source>
        <translation>缺少接收端 IP 或 RTP 連接埠。</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="64"/>
        <location filename="../src/session/gstencoder.cpp" line="250"/>
        <source>no Pulse monitor, video only</source>
        <translation>沒有 Pulse 監聽來源，僅視訊</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="94"/>
        <source>No working encoder. Need a 1.24-compatible mpegtsmux or ffmpeg with libx264.</source>
        <translation>沒有可用的編碼器。需要與 1.24 相容的 mpegtsmux，或含 libx264 的 ffmpeg。</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="212"/>
        <source>gst-launch-1.0 failed to start.</source>
        <translation>gst-launch-1.0 啟動失敗。</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="225"/>
        <source>ffmpeg not found (needed because GStreamer mpegtsmux will not load).</source>
        <translation>找不到 ffmpeg（因 GStreamer mpegtsmux 無法載入而需要它）。</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="277"/>
        <source>ffmpeg failed to start.</source>
        <translation>ffmpeg 啟動失敗。</translation>
    </message>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="317"/>
        <source>encoder exited with code %1</source>
        <translation>編碼器結束，返回碼 %1</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="37"/>
        <source>Miracast</source>
        <translation>無線投屏</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="75"/>
        <source>Monitor</source>
        <translation>顯示器</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="87"/>
        <source>Include system audio</source>
        <translation>包含系統音訊</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="101"/>
        <source>Scan</source>
        <translation>掃描</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="102"/>
        <location filename="../src/ui/mainwindow.cpp" line="285"/>
        <source>Connect</source>
        <translation>連線</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="103"/>
        <source>Disconnect</source>
        <translation>中斷連線</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="122"/>
        <source>Display server: unknown</source>
        <translation>顯示伺服器：未知</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="125"/>
        <source>Display server: X11</source>
        <translation>顯示伺服器：X11</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="128"/>
        <source>Display server: Wayland</source>
        <translation>顯示伺服器：Wayland</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="171"/>
        <source>No Miracast displays found</source>
        <translation>找不到 Miracast 顯示器</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="190"/>
        <source>P2P · no WFD IEs</source>
        <translation>P2P · 無 WFD 資訊元素</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="191"/>
        <source>%1 · no WFD IEs</source>
        <translation>%1 · 無 WFD 資訊元素</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="272"/>
        <source>the display</source>
        <translation>顯示器</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="279"/>
        <source>Enter pairing PIN</source>
        <translation>輸入配對 PIN</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="280"/>
        <source>Enter the PIN shown on %1.</source>
        <translation>請輸入 %1 上顯示的 PIN。</translation>
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
        <translation>請輸入 4 位或 8 位 PIN</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="308"/>
        <source>Confirm pairing</source>
        <translation>確認配對</translation>
    </message>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="309"/>
        <source>Confirm the pairing request on %1.</source>
        <translation>請在 %1 上確認配對請求。</translation>
    </message>
</context>
<context>
    <name>P2PDiscovery</name>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="66"/>
        <source>System D-Bus is not available.</source>
        <translation>系統 D-Bus 無法使用。</translation>
    </message>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="72"/>
        <source>Wi-Fi is off. Turn it on to search for Miracast displays.</source>
        <translation>Wi-Fi 已關閉。請開啟 Wi-Fi 後再搜尋 Miracast 顯示器。</translation>
    </message>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="79"/>
        <source>No Wi-Fi P2P adapter found. Enable Wi-Fi and use NetworkManager with wpa_supplicant P2P (not iwd).</source>
        <translation>找不到 Wi-Fi P2P 介面卡。請啟用 Wi-Fi，並使用搭配 wpa_supplicant P2P 的 NetworkManager（不要使用 iwd）。</translation>
    </message>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="116"/>
        <source>Could not start Wi-Fi Direct find.</source>
        <translation>無法開始 Wi-Fi Direct 搜尋。</translation>
    </message>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="124"/>
        <source>Scanning for Miracast displays…</source>
        <translation>正在掃描 Miracast 顯示器…</translation>
    </message>
</context>
<context>
    <name>P2PSession</name>
    <message>
        <location filename="../src/session/p2psession.cpp" line="67"/>
        <source>Sink is missing P2P address or device path.</source>
        <translation>接收端缺少 P2P 位址或裝置路徑。</translation>
    </message>
    <message>
        <location filename="../src/session/p2psession.cpp" line="116"/>
        <source>Forming Wi-Fi Direct group…</source>
        <translation>正在建立 Wi-Fi Direct 群組…</translation>
    </message>
</context>
<context>
    <name>PortalCapture</name>
    <message>
        <location filename="../src/capture/portalcapture.cpp" line="12"/>
        <source>Screen capture is unavailable on this session. xdg-desktop-portal ScreenCast is not available.</source>
        <translation>目前工作階段無法擷取畫面。xdg-desktop-portal ScreenCast 無法使用。</translation>
    </message>
</context>
<context>
    <name>WfdAudioMode</name>
    <message>
        <location filename="../src/session/wfdaudiomode.cpp" line="37"/>
        <source>none</source>
        <translation>無</translation>
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
        <translation>無法在 RTSP 連接埠 %1 上聆聽：%2</translation>
    </message>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="272"/>
        <source>Waiting for the display on port %1…</source>
        <translation>正在連接埠 %1 上等待顯示器…</translation>
    </message>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="306"/>
        <source>Display connected, starting WFD handshake…</source>
        <translation>顯示器已連線，正在開始 WFD 交握…</translation>
    </message>
</context>
<context>
    <name>WfdSession</name>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="111"/>
        <source>WFD OPTIONS, querying sink…</source>
        <translation>WFD OPTIONS，正在查詢接收端…</translation>
    </message>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="139"/>
        <source>WFD SETUP, RTP port %1</source>
        <translation>WFD SETUP，RTP 連接埠 %1</translation>
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
        <translation>未設定 DISPLAY，無法擷取 X11 畫面。</translation>
    </message>
    <message>
        <location filename="../src/capture/x11capture.cpp" line="18"/>
        <source>No monitor selected.</source>
        <translation>未選擇顯示器。</translation>
    </message>
</context>
</TS>
