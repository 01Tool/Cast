<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_TW">
<context>
    <name>Application</name>
    <message>
        <location filename="../src/main.cpp" line="+29"/>
        <source>Cast</source>
        <translation>投屏</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Cast this screen to a TV or wireless display.</source>
        <translation>將此電腦的畫面投到電視或無線顯示器。</translation>
    </message>
</context>
<context>
    <name>CastEngine</name>
    <message>
        <location filename="../src/engine/castengine.cpp" line="+127"/>
        <source>Enter the pairing PIN for %1…</source>
        <translation>請輸入 %1 的配對 PIN…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Confirm pairing on %1…</source>
        <translation>請在 %1 上確認配對…</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Stop the current session before scanning.</source>
        <translation>請先結束目前工作階段再掃描。</translation>
    </message>
    <message>
        <location line="+33"/>
        <source>Select a display first.</source>
        <translation>請先選擇一台顯示器。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Unknown display.</source>
        <translation>未知顯示器。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Connecting…</source>
        <translation>正在連線…</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>No capture backend for this session.</source>
        <translation>目前工作階段沒有可用的畫面擷取後端。</translation>
    </message>
    <message>
        <location line="+57"/>
        <source>Disconnected.</source>
        <translation>已中斷連線。</translation>
    </message>
    <message>
        <location line="-229"/>
        <source>Idle. Scan to search for Miracast and DLNA displays.</source>
        <translation>空閒。掃描以尋找 Miracast 和 DLNA 顯示器。</translation>
    </message>
    <message>
        <location line="+126"/>
        <location line="+218"/>
        <source>Scanning for Miracast and DLNA displays…</source>
        <translation>正在掃描 Miracast 和 DLNA 顯示器…</translation>
    </message>
    <message>
        <location line="-152"/>
        <source>Trying LAN Miracast (same Wi-Fi as the display)…</source>
        <translation>正在嘗試區域網 Miracast（與顯示器同一 Wi-Fi）…</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>LAN Miracast did not start. Trying Wi-Fi Direct…</source>
        <translation>區域網 Miracast 未能啟動。正在嘗試 Wi-Fi Direct…</translation>
    </message>
    <message>
        <location line="+134"/>
        <source>Found %1 Miracast, %2 DLNA.</source>
        <translation>找到 %1 個 Miracast、%2 個 DLNA。</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>No displays found. Put the TV in Miracast mode, or keep it on this Wi-Fi for DLNA.</source>
        <translation>未找到顯示器。請將電視設為 Miracast 模式，或留在同一 Wi-Fi 上使用 DLNA。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Scan finished. %1 DLNA renderer(s) available.</source>
        <translation>掃描完成。找到 %1 個 DLNA 渲染器。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Found %1 P2P device(s) with no WFD IEs and no DLNA renderers. wpa_supplicant may lack CONFIG_WIFI_DISPLAY.</source>
        <translation>找到 %1 個沒有 WFD IE 的 P2P 裝置，且沒有 DLNA 渲染器。wpa_supplicant 可能未啟用 CONFIG_WIFI_DISPLAY。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Scan finished. %1 Miracast, %2 DLNA.</source>
        <translation>掃描完成。%1 個 Miracast、%2 個 DLNA。</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Timed out waiting for the TV to fetch the HTTP stream. Allow inbound HTTP from the TV to this computer.</source>
        <translation>等待電視擷取 HTTP 串流逾時。請允許電視存取本機的 HTTP。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Timed out waiting for WFD on the LAN. Allow inbound TCP 7236 from the display, then retry.</source>
        <translation>等待區域網上的 WFD 逾時。請允許顯示器存取本機 TCP 7236，然後重試。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Timed out forming the Wi-Fi Direct group or WFD session.</source>
        <translation>建立 Wi-Fi Direct 群組或 WFD 工作階段逾時。</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Wi-Fi Direct group dropped.</source>
        <translation>Wi-Fi Direct 群組已中斷。</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Mirroring %1.</source>
        <translation>正在鏡像 %1。</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Could not send SOURCE_READY on TCP 7250.</source>
        <translation>無法透過 TCP 7250 傳送 SOURCE_READY。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Starting encoder (%1, %2, %3)…</source>
        <translation>正在啟動編碼器（%1，%2，%3）…</translation>
    </message>
    <message>
        <location line="+150"/>
        <source>%1 (%2×%3)%4</source>
        <translation>%1（%2×%3）%4</translation>
    </message>
    <message>
        <location line="+4"/>
        <source> · primary</source>
        <translation> · 主要</translation>
    </message>
</context>
<context>
    <name>CastTrayPlugin</name>
    <message>
        <location filename="../src/tray/casttrayplugin.cpp" line="+38"/>
        <location line="+12"/>
        <source>Cast</source>
        <translation>投屏</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Casting</source>
        <translation>正在投屏</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Scanning</source>
        <translation>正在掃描</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Idle</source>
        <translation>空閒</translation>
    </message>
    <message>
        <location line="+39"/>
        <source>Scan</source>
        <translation>掃描</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Open Cast</source>
        <translation>開啟投屏</translation>
    </message>
</context>
<context>
    <name>DetailWidget</name>
    <message>
        <location filename="../src/tray/detailwidget.cpp" line="+52"/>
        <source>Scan</source>
        <translation>掃描</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Disconnect</source>
        <translation>中斷連線</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Connect</source>
        <translation>連線</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Open Cast</source>
        <translation>開啟投屏</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>Starting Cast…</source>
        <translation>正在啟動投屏…</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Idle. Scan for Miracast and DLNA.</source>
        <translation>空閒。掃描 Miracast 和 DLNA。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>No displays found</source>
        <translation>未找到顯示器</translation>
    </message>
</context>
<context>
    <name>DlnaDiscovery</name>
    <message>
        <location filename="../src/discovery/dlnadiscovery.cpp" line="+77"/>
        <source>Could not listen for DLNA / SSDP replies (%1).</source>
        <translation>無法監聽 DLNA / SSDP 回覆（%1）。</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Scanning for DLNA renderers…</source>
        <translation>正在掃描 DLNA 渲染器…</translation>
    </message>
</context>
<context>
    <name>DlnaSession</name>
    <message>
        <location filename="../src/session/dlnasession.cpp" line="+77"/>
        <source>This display is not a DLNA renderer.</source>
        <translation>此顯示器不是 DLNA 渲染器。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Encoder is missing.</source>
        <translation>缺少編碼器。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>No LAN IPv4 address the TV can reach. Stay on the same Wi-Fi as the renderer.</source>
        <translation>沒有電視能連到的區域網路 IPv4 位址。請與渲染器使用同一 Wi-Fi。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Could not start the HTTP media server (%1).</source>
        <translation>無法啟動 HTTP 媒體伺服器（%1）。</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Offering stream at %1</source>
        <translation>正在透過 %1 提供串流</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Asking %1 which video types it accepts…</source>
        <translation>正在詢問 %1 支援的影片類型…</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>%1 looks file-only (%2). Live MPEG-TS may fail.</source>
        <translation>%1 看起來只支援檔案（%2）。即時 MPEG-TS 可能失敗。</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Sending the stream URL to %1…</source>
        <translation>正在向 %1 傳送串流位址…</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>The TV rejected SetAVTransportURI. It looks file-only (%1), not a live MPEG-TS renderer.</source>
        <translation>電視拒絕了 SetAVTransportURI。它看起來只支援檔案（%1），不是即時 MPEG-TS 渲染器。</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>The TV rejected SetAVTransportURI. It may not play a live MPEG-TS stream.</source>
        <translation>電視拒絕了 SetAVTransportURI。它可能無法播放即時 MPEG-TS 串流。</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>The TV rejected Play.</source>
        <translation>電視拒絕了 Play。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Waiting for %1 to pull the HTTP stream…</source>
        <translation>正在等待 %1 擷取 HTTP 串流…</translation>
    </message>
    <message>
        <location line="+111"/>
        <source>Starting encoder for %1…</source>
        <translation>正在為 %1 啟動編碼器…</translation>
    </message>
</context>
<context>
    <name>GstEncoder</name>
    <message>
        <location filename="../src/session/gstencoder.cpp" line="+43"/>
        <source> from %1</source>
        <translation> 來自 %1</translation>
    </message>
    <message>
        <location line="+66"/>
        <source>Missing sink IP or RTP port.</source>
        <translation>缺少接收端 IP 或 RTP 連接埠。</translation>
    </message>
    <message>
        <location line="-44"/>
        <location line="+221"/>
        <source>no Pulse monitor, video only</source>
        <translation>沒有 Pulse 監聽來源，僅視訊</translation>
    </message>
    <message>
        <location line="-187"/>
        <source>No working encoder. Need a 1.24-compatible mpegtsmux or ffmpeg with libx264.</source>
        <translation>沒有可用的編碼器。需要與 1.24 相容的 mpegtsmux，或含 libx264 的 ffmpeg。</translation>
    </message>
    <message>
        <location line="+150"/>
        <source>gst-launch-1.0 failed to start.</source>
        <translation>gst-launch-1.0 啟動失敗。</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>ffmpeg not found (needed because GStreamer mpegtsmux will not load).</source>
        <translation>找不到 ffmpeg（因 GStreamer mpegtsmux 無法載入而需要它）。</translation>
    </message>
    <message>
        <location line="+61"/>
        <source>ffmpeg failed to start.</source>
        <translation>ffmpeg 啟動失敗。</translation>
    </message>
    <message>
        <location line="+40"/>
        <source>encoder exited with code %1</source>
        <translation>編碼器結束，返回碼 %1</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../src/ui/mainwindow.cpp" line="+37"/>
        <source>Cast</source>
        <translation>投屏</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>Monitor</source>
        <translation>顯示器</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Include system audio</source>
        <translation>包含系統音訊</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Scan</source>
        <translation>掃描</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+206"/>
        <source>Connect</source>
        <translation>連線</translation>
    </message>
    <message>
        <location line="-205"/>
        <source>Disconnect</source>
        <translation>中斷連線</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Display server: unknown</source>
        <translation>顯示伺服器：未知</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Display server: X11</source>
        <translation>顯示伺服器：X11</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Display server: Wayland</source>
        <translation>顯示伺服器：Wayland</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>No displays found</source>
        <translation>未找到顯示器</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>DLNA</source>
        <translation>DLNA</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Miracast</source>
        <translation>Miracast</translation>
    </message>
    <message>
        <location line="+3"/>
        <source> · LAN</source>
        <translation> · 區域網</translation>
    </message>
    <message>
        <location line="+5"/>
        <source> · no WFD IEs</source>
        <translation> · 無 WFD IE</translation>
    </message>
    <message>
        <location line="+4"/>
        <source> · live TS likely</source>
        <translation> · 可能支援即時 TS</translation>
    </message>
    <message>
        <location line="+3"/>
        <source> · files only</source>
        <translation> · 僅檔案</translation>
    </message>
    <message>
        <location line="+3"/>
        <source> · no video</source>
        <translation> · 無影片</translation>
    </message>
    <message>
        <location line="+88"/>
        <source>the display</source>
        <translation>顯示器</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Enter pairing PIN</source>
        <translation>輸入配對 PIN</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Enter the PIN shown on %1.</source>
        <translation>請輸入 %1 上顯示的 PIN。</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>4 or 8 digit PIN</source>
        <translation>4 位或 8 位 PIN</translation>
    </message>
    <message>
        <location line="+4"/>
        <location line="+24"/>
        <source>Cancel</source>
        <translation>取消</translation>
    </message>
    <message>
        <location line="-9"/>
        <source>Enter a 4- or 8-digit PIN</source>
        <translation>請輸入 4 位或 8 位 PIN</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Confirm pairing</source>
        <translation>確認配對</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Confirm the pairing request on %1.</source>
        <translation>請在 %1 上確認配對請求。</translation>
    </message>
</context>
<context>
    <name>MiceDiscovery</name>
    <message>
        <location filename="../src/discovery/micediscovery.cpp" line="+117"/>
        <source>Scanning for LAN Miracast (Windows Connect)…</source>
        <translation>正在掃描區域網 Miracast（Windows 連線）…</translation>
    </message>
</context>
<context>
    <name>MiceSession</name>
    <message>
        <location filename="../src/session/micesession.cpp" line="+34"/>
        <source>The display closed the LAN Miracast (MS-MICE) channel.</source>
        <translation>顯示器關閉了區域網 Miracast（MS-MICE）通道。</translation>
    </message>
    <message>
        <location line="+54"/>
        <source>No LAN address for this Miracast display.</source>
        <translation>此 Miracast 顯示器沒有區域網位址。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Resolving %1 on the LAN…</source>
        <translation>正在解析區域網上的 %1…</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Could not resolve a LAN address for %1.</source>
        <translation>無法解析 %1 的區域網位址。</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Opening Windows Connect on %1:%2…</source>
        <translation>正在連線 %1:%2（Windows 連線）…</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>No LAN IPv4 address the display can reach.</source>
        <translation>沒有顯示器能存取的區域網 IPv4 位址。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>LAN Miracast channel open. Waiting for WFD…</source>
        <translation>區域網 Miracast 通道已開啟。正在等待 WFD…</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>No listener on TCP %1 (MS-MICE).</source>
        <translation>TCP %1 上沒有監聽（MS-MICE）。</translation>
    </message>
    <message>
        <location line="+30"/>
        <source>Could not send SOURCE_READY to the display.</source>
        <translation>無法向顯示器傳送 SOURCE_READY。</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Told the display we are ready on RTSP %1…</source>
        <translation>已通知顯示器在 RTSP %1 上就緒…</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>The display asked for a Miracast option this sender does not implement yet (MS-MICE command %1).</source>
        <translation>顯示器請求了本傳送端尚未實作的 Miracast 選項（MS-MICE 命令 %1）。</translation>
    </message>
</context>
<context>
    <name>P2PDiscovery</name>
    <message>
        <location filename="../src/discovery/p2pdiscovery.cpp" line="+66"/>
        <source>System D-Bus is not available.</source>
        <translation>系統 D-Bus 無法使用。</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Wi-Fi is off. Turn it on to search for Miracast displays.</source>
        <translation>Wi-Fi 已關閉。請開啟 Wi-Fi 後再搜尋 Miracast 顯示器。</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>No Wi-Fi P2P adapter found. Enable Wi-Fi and use NetworkManager with wpa_supplicant P2P (not iwd).</source>
        <translation>找不到 Wi-Fi P2P 介面卡。請啟用 Wi-Fi，並使用搭配 wpa_supplicant P2P 的 NetworkManager（不要使用 iwd）。</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>Could not start Wi-Fi Direct find.</source>
        <translation>無法開始 Wi-Fi Direct 搜尋。</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Scanning for Miracast displays…</source>
        <translation>正在掃描 Miracast 顯示器…</translation>
    </message>
</context>
<context>
    <name>P2PSession</name>
    <message>
        <location filename="../src/session/p2psession.cpp" line="+69"/>
        <source>Sink is missing P2P address or device path.</source>
        <translation>接收端缺少 P2P 位址或裝置路徑。</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>Forming Wi-Fi Direct group…</source>
        <translation>正在建立 Wi-Fi Direct 群組…</translation>
    </message>
</context>
<context>
    <name>PortalCapture</name>
    <message>
        <location filename="../src/capture/portalcapture.cpp" line="+12"/>
        <source>Screen capture is unavailable on this session. xdg-desktop-portal ScreenCast is not available.</source>
        <translation>目前工作階段無法擷取畫面。xdg-desktop-portal ScreenCast 無法使用。</translation>
    </message>
</context>
<context>
    <name>QuickPanel</name>
    <message>
        <location filename="../src/tray/quickpanel.cpp" line="+31"/>
        <source>Cast</source>
        <translation>投屏</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Idle</source>
        <translation>空閒</translation>
    </message>
</context>
<context>
    <name>WfdAudioMode</name>
    <message>
        <location filename="../src/session/wfdaudiomode.cpp" line="+37"/>
        <source>none</source>
        <translation>無</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>AAC %1 kHz</source>
        <translation>AAC %1 kHz</translation>
    </message>
</context>
<context>
    <name>WfdServer</name>
    <message>
        <location filename="../src/session/wfdserver.cpp" line="+266"/>
        <source>Cannot listen on RTSP port %1: %2</source>
        <translation>無法在 RTSP 連接埠 %1 上聆聽：%2</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Waiting for the display on port %1…</source>
        <translation>正在連接埠 %1 上等待顯示器…</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>Display connected, starting WFD handshake…</source>
        <translation>顯示器已連線，正在開始 WFD 交握…</translation>
    </message>
</context>
<context>
    <name>WfdSession</name>
    <message>
        <location line="-195"/>
        <source>WFD OPTIONS, querying sink…</source>
        <translation>WFD OPTIONS，正在查詢接收端…</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>WFD SETUP, RTP port %1</source>
        <translation>WFD SETUP，RTP 連接埠 %1</translation>
    </message>
    <message>
        <location line="+72"/>
        <source>WFD GET_PARAMETER…</source>
        <translation>WFD GET_PARAMETER…</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>WFD SET_PARAMETER %1, %2…</source>
        <translation>WFD SET_PARAMETER %1，%2…</translation>
    </message>
</context>
<context>
    <name>X11Capture</name>
    <message>
        <location filename="../src/capture/x11capture.cpp" line="+13"/>
        <source>DISPLAY is not set; cannot grab the X11 screen.</source>
        <translation>未設定 DISPLAY，無法擷取 X11 畫面。</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>No monitor selected.</source>
        <translation>未選擇顯示器。</translation>
    </message>
</context>
</TS>
