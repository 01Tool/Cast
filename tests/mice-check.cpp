#include "session/miceprotocol.h"

#include <QByteArray>
#include <cstdio>

static int g_failed = 0;

static void expectTrue(const char *name, bool ok)
{
    if (ok)
        return;
    std::fprintf(stderr, "FAIL %s\n", name);
    ++g_failed;
}

static void expectEq(const char *name, const QString &got, const QString &want)
{
    if (got == want)
        return;
    std::fprintf(stderr, "FAIL %s: got [%s] want [%s]\n", name, qPrintable(got), qPrintable(want));
    ++g_failed;
}

static void expectInt(const char *name, int got, int want)
{
    if (got == want)
        return;
    std::fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
    ++g_failed;
}

static void appendBe16(QByteArray &out, quint16 value)
{
    out.append(char(value >> 8));
    out.append(char(value & 0xff));
}

static void appendLabel(QByteArray &out, const char *label)
{
    const QByteArray bytes(label);
    out.append(char(bytes.size()));
    out.append(bytes);
}

int main()
{
    const QByteArray sourceId = QByteArray::fromHex("91f4abe9eff5464aaee269722aed11b5");
    const QByteArray ready = encodeMiceSourceReady(QStringLiteral("Dummy1-Kabylake"), sourceId, 7236);
    expectInt("source-ready size field",
              (quint8(ready.at(0)) << 8) | quint8(ready.at(1)), ready.size());
    expectInt("source-ready version", quint8(ready.at(2)), 1);
    expectInt("source-ready command", quint8(ready.at(3)), 1);
    expectTrue("source-ready has friendly-name tlv", quint8(ready.at(4)) == 0);
    const int nameLen = (quint8(ready.at(5)) << 8) | quint8(ready.at(6));
    expectInt("utf16 name bytes", nameLen, 30);
    expectTrue("utf16le D", quint8(ready.at(7)) == 0x44 && quint8(ready.at(8)) == 0x00);
    expectTrue("contains rtsp tlv", ready.contains(QByteArray::fromHex("0200021c44")));
    expectTrue("contains source id", ready.contains(sourceId));
    expectInt("source-ready total", ready.size(), 61);

    const QByteArray stop = encodeMiceStopProjection(QStringLiteral("Dummy1-Kabylake"), sourceId);
    expectInt("stop command", quint8(stop.at(3)), 2);
    expectTrue("stop has no rtsp tlv", !stop.contains(QByteArray::fromHex("0200021c44")));

    expectEq("norm mac", normalizeMac(QStringLiteral("76:12:B3:C8:64:AB")),
             QStringLiteral("7612b3c864ab"));
    expectEq("laa toggle", toggleMacLaa(QStringLiteral("76:12:B3:C8:64:AB")),
             QStringLiteral("7412b3c864ab"));
    expectTrue("macs related LAA",
               macsRelated(QStringLiteral("76:12:B3:C8:64:AB"),
                           QStringLiteral("74:12:b3:c8:64:ab")));
    expectTrue("macs not related",
               !macsRelated(QStringLiteral("76:12:B3:C8:64:AB"),
                            QStringLiteral("aa:bb:cc:dd:ee:ff")));

    const QByteArray arp =
        "IP address       HW type     Flags       HW address            Mask     Device\n"
        "192.168.196.16   0x1         0x2         74:12:b3:c8:64:ab     *        wlp4s0\n"
        "192.168.196.1    0x1         0x2         11:22:33:44:55:66     *        wlp4s0\n";
    expectEq("arp from p2p mac",
             ipv4ForHardwareAddress(QStringLiteral("76:12:B3:C8:64:AB"), arp),
             QStringLiteral("192.168.196.16"));
    expectEq("arp miss", ipv4ForHardwareAddress(QStringLiteral("00:11:22:33:44:55"), arp),
             QString());

    const QByteArray query = buildMdnsDisplayPtrQuery();
    expectTrue("mdns ptr _display", query.contains(QByteArray("_display")));
    expectTrue("mdns ptr _tcp", query.contains(QByteArray("_tcp")));
    expectTrue("mdns ptr local", query.contains(QByteArray("local")));
    expectTrue("mdns QU bit", query.endsWith(QByteArray::fromHex("000c8001")));

    QByteArray mdns;
    appendBe16(mdns, 0);
    appendBe16(mdns, 0x8400);
    appendBe16(mdns, 0);
    appendBe16(mdns, 1);
    appendBe16(mdns, 0);
    appendBe16(mdns, 3);
    appendLabel(mdns, "_display");
    appendLabel(mdns, "_tcp");
    appendLabel(mdns, "local");
    mdns.append('\0');
    appendBe16(mdns, 12);
    appendBe16(mdns, 1);
    mdns.append(char(0));
    mdns.append(char(0));
    mdns.append(char(0));
    mdns.append(char(1));
    const int ptrRdata = mdns.size();
    appendBe16(mdns, 0);
    appendLabel(mdns, "TestPC");
    mdns.append(char(0xc0));
    mdns.append(char(12));
    const quint16 ptrLen = quint16(mdns.size() - ptrRdata - 2);
    mdns[ptrRdata] = char(ptrLen >> 8);
    mdns[ptrRdata + 1] = char(ptrLen & 0xff);

    appendLabel(mdns, "TestPC");
    mdns.append(char(0xc0));
    mdns.append(char(12));
    appendBe16(mdns, 33);
    appendBe16(mdns, 1);
    mdns.append(char(0));
    mdns.append(char(0));
    mdns.append(char(0));
    mdns.append(char(1));
    const int srvRdata = mdns.size();
    appendBe16(mdns, 0);
    appendBe16(mdns, 0);
    appendBe16(mdns, 0);
    appendBe16(mdns, 7250);
    appendLabel(mdns, "TestPC");
    appendLabel(mdns, "local");
    mdns.append('\0');
    const quint16 srvLen = quint16(mdns.size() - srvRdata - 2);
    mdns[srvRdata] = char(srvLen >> 8);
    mdns[srvRdata + 1] = char(srvLen & 0xff);

    appendLabel(mdns, "TestPC");
    appendLabel(mdns, "local");
    mdns.append('\0');
    appendBe16(mdns, 1);
    appendBe16(mdns, 1);
    mdns.append(char(0));
    mdns.append(char(0));
    mdns.append(char(0));
    mdns.append(char(1));
    appendBe16(mdns, 4);
    mdns.append(char(192));
    mdns.append(char(168));
    mdns.append(char(196));
    mdns.append(char(16));

    appendLabel(mdns, "TestPC");
    mdns.append(char(0xc0));
    mdns.append(char(12));
    appendBe16(mdns, 16);
    appendBe16(mdns, 1);
    mdns.append(char(0));
    mdns.append(char(0));
    mdns.append(char(0));
    mdns.append(char(1));
    const QByteArray txtValue = QByteArray("p2pMAC=76:12:b3:c8:64:ab");
    QByteArray txt;
    txt.append(char(txtValue.size()));
    txt.append(txtValue);
    appendBe16(mdns, quint16(txt.size()));
    mdns.append(txt);

    const auto services = parseMdnsDisplayServices(mdns, QStringLiteral("192.168.196.99"));
    expectInt("mdns service count", services.size(), 1);
    if (!services.isEmpty()) {
        expectEq("mdns name", services.at(0).name, QStringLiteral("TestPC"));
        expectEq("mdns ipv4", services.at(0).ipv4, QStringLiteral("192.168.196.16"));
        expectInt("mdns port", services.at(0).port, 7250);
        expectEq("mdns p2pMAC", services.at(0).p2pMac.toLower(),
                 QStringLiteral("76:12:b3:c8:64:ab"));
    }

    if (g_failed) {
        std::fprintf(stderr, "%d checks failed\n", g_failed);
        return 1;
    }
    std::fprintf(stdout, "mice-check ok\n");
    return 0;
}
