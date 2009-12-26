#include "XmlRpcMethod.h"

#include <cppunit/extensions/HelperMacros.h>

#include "DownloadEngine.h"
#include "SelectEventPoll.h"
#include "Option.h"
#include "RequestGroupMan.h"
#include "ServerStatMan.h"
#include "RequestGroup.h"
#include "XmlRpcMethodImpl.h"
#include "BDE.h"
#include "OptionParser.h"
#include "OptionHandler.h"
#include "XmlRpcRequest.h"
#include "XmlRpcResponse.h"
#include "prefs.h"
#include "TestUtil.h"
#include "DownloadContext.h"
#include "FeatureConfig.h"
#include "util.h"
#ifdef ENABLE_BITTORRENT
# include "BtRegistry.h"
# include "BtRuntime.h"
# include "PieceStorage.h"
# include "PeerStorage.h"
# include "BtProgressInfoFile.h"
# include "BtAnnounce.h"
#endif // ENABLE_BITTORRENT

namespace aria2 {

namespace xmlrpc {

class XmlRpcMethodTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(XmlRpcMethodTest);
  CPPUNIT_TEST(testAddUri);
  CPPUNIT_TEST(testAddUri_withoutUri);
  CPPUNIT_TEST(testAddUri_notUri);
  CPPUNIT_TEST(testAddUri_withBadOption);
  CPPUNIT_TEST(testAddUri_withPosition);
  CPPUNIT_TEST(testAddUri_withBadPosition);
#ifdef ENABLE_BITTORRENT
  CPPUNIT_TEST(testAddTorrent);
  CPPUNIT_TEST(testAddTorrent_withoutTorrent);
  CPPUNIT_TEST(testAddTorrent_notBase64Torrent);
  CPPUNIT_TEST(testAddTorrent_withPosition);
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
  CPPUNIT_TEST(testAddMetalink);
  CPPUNIT_TEST(testAddMetalink_withoutMetalink);
  CPPUNIT_TEST(testAddMetalink_notBase64Metalink);
  CPPUNIT_TEST(testAddMetalink_withPosition);
#endif // ENABLE_METALINK
  CPPUNIT_TEST(testChangeOption);
  CPPUNIT_TEST(testChangeOption_withBadOption);
  CPPUNIT_TEST(testChangeOption_withNotAllowedOption);
  CPPUNIT_TEST(testChangeOption_withoutGid);
  CPPUNIT_TEST(testChangeGlobalOption);
  CPPUNIT_TEST(testChangeGlobalOption_withBadOption);
  CPPUNIT_TEST(testChangeGlobalOption_withNotAllowedOption);
  CPPUNIT_TEST(testTellStatus_withoutGid);
  CPPUNIT_TEST(testTellWaiting);
  CPPUNIT_TEST(testTellWaiting_fail);
  CPPUNIT_TEST(testGetVersion);
  CPPUNIT_TEST(testNoSuchMethod);
  CPPUNIT_TEST(testGatherStoppedDownload);
  CPPUNIT_TEST(testGatherProgressCommon);
  CPPUNIT_TEST(testChangePosition);
  CPPUNIT_TEST(testChangePosition_fail);
  CPPUNIT_TEST(testSystemMulticall);
  CPPUNIT_TEST(testSystemMulticall_fail);
  CPPUNIT_TEST_SUITE_END();
private:
  SharedHandle<DownloadEngine> _e;
  SharedHandle<Option> _option;
public:
  void setUp()
  {
    RequestGroup::resetGIDCounter();
    _option.reset(new Option());
    _option->put(PREF_DIR, "/tmp");
    _option->put(PREF_SEGMENT_SIZE, "1048576");
    _e.reset(new DownloadEngine(SharedHandle<EventPoll>(new SelectEventPoll())));
    _e->option = _option.get();
    _e->_requestGroupMan.reset
      (new RequestGroupMan(std::deque<SharedHandle<RequestGroup> >(),
			   1, _option.get()));
  }

  void tearDown() {}

  void testAddUri();
  void testAddUri_withoutUri();
  void testAddUri_notUri();
  void testAddUri_withBadOption();
  void testAddUri_withPosition();
  void testAddUri_withBadPosition();
#ifdef ENABLE_BITTORRENT
  void testAddTorrent();
  void testAddTorrent_withoutTorrent();
  void testAddTorrent_notBase64Torrent();
  void testAddTorrent_withPosition();
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
  void testAddMetalink();
  void testAddMetalink_withoutMetalink();
  void testAddMetalink_notBase64Metalink();
  void testAddMetalink_withPosition();
#endif // ENABLE_METALINK
  void testChangeOption();
  void testChangeOption_withBadOption();
  void testChangeOption_withNotAllowedOption();
  void testChangeOption_withoutGid();
  void testChangeGlobalOption();
  void testChangeGlobalOption_withBadOption();
  void testChangeGlobalOption_withNotAllowedOption();
  void testTellStatus_withoutGid();
  void testTellWaiting();
  void testTellWaiting_fail();
  void testGetVersion();
  void testNoSuchMethod();
  void testGatherStoppedDownload();
  void testGatherProgressCommon();
  void testChangePosition();
  void testChangePosition_fail();
  void testSystemMulticall();
  void testSystemMulticall_fail();
};


CPPUNIT_TEST_SUITE_REGISTRATION(XmlRpcMethodTest);

void XmlRpcMethodTest::testAddUri()
{
  AddUriXmlRpcMethod m;
  XmlRpcRequest req(AddUriXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE::list();
  req._params[0] << BDE("http://localhost/");
  {
    XmlRpcResponse res = m.execute(req, _e.get());
    CPPUNIT_ASSERT_EQUAL(0, res._code);
    const std::deque<SharedHandle<RequestGroup> > rgs =
      _e->_requestGroupMan->getReservedGroups();
    CPPUNIT_ASSERT_EQUAL((size_t)1, rgs.size());
    CPPUNIT_ASSERT_EQUAL(std::string("http://localhost/"),
			 rgs.front()->getDownloadContext()->getFirstFileEntry()->getRemainingUris().front());
  }
  // with options
  BDE opt = BDE::dict();
  opt[PREF_DIR] = BDE("/sink");
  req._params << opt;
  {
    XmlRpcResponse res = m.execute(req, _e.get());
    CPPUNIT_ASSERT_EQUAL(0, res._code);
    CPPUNIT_ASSERT_EQUAL(std::string("/sink"),
			 _e->_requestGroupMan->findReservedGroup(2)->
			 getDownloadContext()->getDir());
  }
}

void XmlRpcMethodTest::testAddUri_withoutUri()
{
  AddUriXmlRpcMethod m;
  XmlRpcRequest req(AddUriXmlRpcMethod::getMethodName(), BDE::list());
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testAddUri_notUri()
{
  AddUriXmlRpcMethod m;
  XmlRpcRequest req(AddUriXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE::list();
  req._params[0] << BDE("not uri");
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testAddUri_withBadOption()
{
  AddUriXmlRpcMethod m;
  XmlRpcRequest req(AddUriXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE::list();
  req._params[0] << BDE("http://localhost");
  BDE opt = BDE::dict();
  opt[PREF_FILE_ALLOCATION] = BDE("badvalue");
  req._params << opt;
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testAddUri_withPosition()
{
  AddUriXmlRpcMethod m;
  XmlRpcRequest req1(AddUriXmlRpcMethod::getMethodName(), BDE::list());
  req1._params << BDE::list();
  req1._params[0] << BDE("http://uri1");
  XmlRpcResponse res1 = m.execute(req1, _e.get());
  CPPUNIT_ASSERT_EQUAL(0, res1._code);
  
  XmlRpcRequest req2(AddUriXmlRpcMethod::getMethodName(), BDE::list());
  req2._params << BDE::list();
  req2._params[0] << BDE("http://uri2");
  req2._params << BDE::dict();
  req2._params << BDE((int64_t)0);
  m.execute(req2, _e.get());

  std::string uri =
    _e->_requestGroupMan->getReservedGroups()[0]->getDownloadContext()->getFirstFileEntry()->getRemainingUris()[0];

  CPPUNIT_ASSERT_EQUAL(std::string("http://uri2"), uri);
}

void XmlRpcMethodTest::testAddUri_withBadPosition()
{
  AddUriXmlRpcMethod m;
  XmlRpcRequest req(AddUriXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE::list();
  req._params[0] << BDE("http://localhost/");
  req._params << BDE::dict();
  req._params << BDE((int64_t)-1);
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

#ifdef ENABLE_BITTORRENT
void XmlRpcMethodTest::testAddTorrent()
{
  AddTorrentXmlRpcMethod m;
  XmlRpcRequest req(AddTorrentXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE(readFile("single.torrent"));
  BDE uris = BDE::list();
  uris << BDE("http://localhost/aria2-0.8.2.tar.bz2");
  req._params << uris;
  {
    XmlRpcResponse res = m.execute(req, _e.get());
    CPPUNIT_ASSERT_EQUAL(0, res._code);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), res._param.s());

    SharedHandle<RequestGroup> group = _e->_requestGroupMan->findReservedGroup(1);
    CPPUNIT_ASSERT(!group.isNull());
    CPPUNIT_ASSERT_EQUAL(std::string("/tmp/aria2-0.8.2.tar.bz2"),
			 group->getFirstFilePath());
    CPPUNIT_ASSERT_EQUAL((size_t)1, group->getDownloadContext()->getFirstFileEntry()->getRemainingUris().size());
    CPPUNIT_ASSERT_EQUAL(std::string("http://localhost/aria2-0.8.2.tar.bz2"),
			 group->getDownloadContext()->getFirstFileEntry()->getRemainingUris()[0]);
  }
  // with options
  BDE opt = BDE::dict();
  opt[PREF_DIR] = BDE("/sink");
  req._params << opt;
  {
    XmlRpcResponse res = m.execute(req, _e.get());
    CPPUNIT_ASSERT_EQUAL(0, res._code);
    CPPUNIT_ASSERT_EQUAL(std::string("/sink/aria2-0.8.2.tar.bz2"),
			 _e->_requestGroupMan->findReservedGroup(2)->getFirstFilePath());
  }
}

void XmlRpcMethodTest::testAddTorrent_withoutTorrent()
{
  AddTorrentXmlRpcMethod m;
  XmlRpcRequest req(AddTorrentXmlRpcMethod::getMethodName(), BDE::list());
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testAddTorrent_notBase64Torrent()
{
  AddTorrentXmlRpcMethod m;
  XmlRpcRequest req(AddTorrentXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE("not torrent");
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testAddTorrent_withPosition()
{
  AddTorrentXmlRpcMethod m;
  XmlRpcRequest req1(AddTorrentXmlRpcMethod::getMethodName(), BDE::list());
  req1._params << BDE(readFile("test.torrent"));
  req1._params << BDE::list();
  req1._params << BDE::dict();
  XmlRpcResponse res1 = m.execute(req1, _e.get());
  CPPUNIT_ASSERT_EQUAL(0, res1._code);

  XmlRpcRequest req2(AddTorrentXmlRpcMethod::getMethodName(), BDE::list());
  req2._params << BDE(readFile("single.torrent"));
  req2._params << BDE::list();
  req2._params << BDE::dict();
  req2._params << BDE((int64_t)0);
  m.execute(req2, _e.get());

  CPPUNIT_ASSERT_EQUAL((size_t)1,
		       _e->_requestGroupMan->getReservedGroups()[0]->
		       getDownloadContext()->getFileEntries().size());
}

#endif // ENABLE_BITTORRENT

#ifdef ENABLE_METALINK
void XmlRpcMethodTest::testAddMetalink()
{
  AddMetalinkXmlRpcMethod m;
  XmlRpcRequest req(AddMetalinkXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE(readFile("2files.metalink"));
  {
    XmlRpcResponse res = m.execute(req, _e.get());
    CPPUNIT_ASSERT_EQUAL(0, res._code);
    CPPUNIT_ASSERT_EQUAL((size_t)2, res._param.size());
    CPPUNIT_ASSERT_EQUAL(std::string("1"), res._param[0].s());
    CPPUNIT_ASSERT_EQUAL(std::string("2"), res._param[1].s());

    SharedHandle<RequestGroup> tar = _e->_requestGroupMan->findReservedGroup(1);
    CPPUNIT_ASSERT(!tar.isNull());
    CPPUNIT_ASSERT_EQUAL(std::string("/tmp/aria2-5.0.0.tar.bz2"),
			 tar->getFirstFilePath());
    SharedHandle<RequestGroup> deb = _e->_requestGroupMan->findReservedGroup(2);
    CPPUNIT_ASSERT(!deb.isNull());
    CPPUNIT_ASSERT_EQUAL(std::string("/tmp/aria2-5.0.0.deb"),
			 deb->getFirstFilePath());
  }
  // with options
  BDE opt = BDE::dict();
  opt[PREF_DIR] = BDE("/sink");
  req._params << opt;
  {
    XmlRpcResponse res = m.execute(req, _e.get());
    CPPUNIT_ASSERT_EQUAL(0, res._code);
    CPPUNIT_ASSERT_EQUAL(std::string("/sink/aria2-5.0.0.tar.bz2"),
			 _e->_requestGroupMan->findReservedGroup(3)->getFirstFilePath());
  }
}

void XmlRpcMethodTest::testAddMetalink_withoutMetalink()
{
  AddMetalinkXmlRpcMethod m;
  XmlRpcRequest req(AddMetalinkXmlRpcMethod::getMethodName(), BDE::list());
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testAddMetalink_notBase64Metalink()
{
  AddMetalinkXmlRpcMethod m;
  XmlRpcRequest req(AddMetalinkXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE("not metalink");
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testAddMetalink_withPosition()
{
  AddUriXmlRpcMethod m1;
  XmlRpcRequest req1(AddUriXmlRpcMethod::getMethodName(), BDE::list());
  req1._params << BDE::list();
  req1._params[0] << BDE("http://uri");
  XmlRpcResponse res1 = m1.execute(req1, _e.get());
  CPPUNIT_ASSERT_EQUAL(0, res1._code);

  AddMetalinkXmlRpcMethod m2;
  XmlRpcRequest req2("ari2.addMetalink", BDE::list());
  req2._params << BDE(readFile("2files.metalink"));
  req2._params << BDE::dict();
  req2._params << BDE((int64_t)0);
  XmlRpcResponse res2 = m2.execute(req2, _e.get());
  CPPUNIT_ASSERT_EQUAL(0, res2._code);

  CPPUNIT_ASSERT_EQUAL(std::string("/tmp/aria2-5.0.0.tar.bz2"),
		       _e->_requestGroupMan->getReservedGroups()[0]->
		       getFirstFilePath());
}

#endif // ENABLE_METALINK

void XmlRpcMethodTest::testChangeOption()
{
  SharedHandle<RequestGroup> group(new RequestGroup(_option));
  _e->_requestGroupMan->addReservedGroup(group);

  ChangeOptionXmlRpcMethod m;
  XmlRpcRequest req(ChangeOptionXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE("1");
  BDE opt = BDE::dict();
  opt[PREF_MAX_DOWNLOAD_LIMIT] = BDE("100K");
#ifdef ENABLE_BITTORRENT
  opt[PREF_BT_MAX_PEERS] = BDE("100");
  opt[PREF_BT_REQUEST_PEER_SPEED_LIMIT] = BDE("300K");
  opt[PREF_MAX_UPLOAD_LIMIT] = BDE("50K");

  BtObject btObject;
  btObject._btRuntime = SharedHandle<BtRuntime>(new BtRuntime());
  _e->getBtRegistry()->put(group->getGID(), btObject);
#endif // ENABLE_BITTORRENT
  req._params << opt;
  XmlRpcResponse res = m.execute(req, _e.get());

  SharedHandle<Option> option = group->getOption();

  CPPUNIT_ASSERT_EQUAL(0, res._code);
  CPPUNIT_ASSERT_EQUAL((unsigned int)100*1024,
		       group->getMaxDownloadSpeedLimit());
  CPPUNIT_ASSERT_EQUAL(std::string("102400"),
		       option->get(PREF_MAX_DOWNLOAD_LIMIT));
#ifdef ENABLE_BITTORRENT
  CPPUNIT_ASSERT_EQUAL(std::string("307200"),
		       option->get(PREF_BT_REQUEST_PEER_SPEED_LIMIT));

  CPPUNIT_ASSERT_EQUAL(std::string("100"), option->get(PREF_BT_MAX_PEERS));
  CPPUNIT_ASSERT_EQUAL((unsigned int)100, btObject._btRuntime->getMaxPeers());

  CPPUNIT_ASSERT_EQUAL((unsigned int)50*1024,
		       group->getMaxUploadSpeedLimit());
  CPPUNIT_ASSERT_EQUAL(std::string("51200"),
		       option->get(PREF_MAX_UPLOAD_LIMIT));
#endif // ENABLE_BITTORRENT
}

void XmlRpcMethodTest::testChangeOption_withBadOption()
{
  SharedHandle<RequestGroup> group(new RequestGroup(_option));
  _e->_requestGroupMan->addReservedGroup(group);

  ChangeOptionXmlRpcMethod m;
  XmlRpcRequest req(ChangeOptionXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE("1");
  BDE opt = BDE::dict();
  opt[PREF_MAX_DOWNLOAD_LIMIT] = BDE("badvalue");
  req._params << opt;
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testChangeOption_withNotAllowedOption()
{
  SharedHandle<RequestGroup> group(new RequestGroup(_option));
  _e->_requestGroupMan->addReservedGroup(group);

  ChangeOptionXmlRpcMethod m;
  XmlRpcRequest req(ChangeOptionXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE("1");
  BDE opt = BDE::dict();
  opt[PREF_MAX_OVERALL_DOWNLOAD_LIMIT] = BDE("100K");
  req._params << opt;
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testChangeOption_withoutGid()
{
  ChangeOptionXmlRpcMethod m;
  XmlRpcRequest req(ChangeOptionXmlRpcMethod::getMethodName(), BDE::list());
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testChangeGlobalOption()
{
  ChangeGlobalOptionXmlRpcMethod m;
  XmlRpcRequest req(ChangeGlobalOptionXmlRpcMethod::getMethodName(), BDE::list());
  BDE opt = BDE::dict();
  opt[PREF_MAX_OVERALL_DOWNLOAD_LIMIT] = BDE("100K");
#ifdef ENABLE_BITTORRENT
  opt[PREF_MAX_OVERALL_UPLOAD_LIMIT] = BDE("50K");
#endif // ENABLE_BITTORRENT
  req._params << opt;
  XmlRpcResponse res = m.execute(req, _e.get());

  CPPUNIT_ASSERT_EQUAL(0, res._code);
  CPPUNIT_ASSERT_EQUAL((unsigned int)100*1024,
		       _e->_requestGroupMan->getMaxOverallDownloadSpeedLimit());
  CPPUNIT_ASSERT_EQUAL(std::string("102400"),
		       _e->option->get(PREF_MAX_OVERALL_DOWNLOAD_LIMIT));
#ifdef ENABLE_BITTORRENT
  CPPUNIT_ASSERT_EQUAL((unsigned int)50*1024,
		       _e->_requestGroupMan->getMaxOverallUploadSpeedLimit());
  CPPUNIT_ASSERT_EQUAL(std::string("51200"),
		       _e->option->get(PREF_MAX_OVERALL_UPLOAD_LIMIT));
#endif // ENABLE_BITTORRENT
}

void XmlRpcMethodTest::testChangeGlobalOption_withBadOption()
{
  ChangeGlobalOptionXmlRpcMethod m;
  XmlRpcRequest req(ChangeGlobalOptionXmlRpcMethod::getMethodName(), BDE::list());
  BDE opt = BDE::dict();
  opt[PREF_MAX_OVERALL_DOWNLOAD_LIMIT] = BDE("badvalue");
  req._params << opt;
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testChangeGlobalOption_withNotAllowedOption()
{
  ChangeGlobalOptionXmlRpcMethod m;
  XmlRpcRequest req(ChangeGlobalOptionXmlRpcMethod::getMethodName(), BDE::list());
  BDE opt = BDE::dict();
  opt[PREF_MAX_DOWNLOAD_LIMIT] = BDE("100K");
  req._params << opt;
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testNoSuchMethod()
{
  NoSuchMethodXmlRpcMethod m;
  XmlRpcRequest req("make.hamburger", BDE::none);
  XmlRpcResponse res = m.execute(req, 0);
  CPPUNIT_ASSERT_EQUAL(1, res._code);
  CPPUNIT_ASSERT_EQUAL(std::string("No such method: make.hamburger"),
		       res._param["faultString"].s());
  CPPUNIT_ASSERT_EQUAL
    (std::string("<?xml version=\"1.0\"?>"
		 "<methodResponse>"
		 "<fault>"
		 "<value>"
		 "<struct>"
		 "<member>"
		 "<name>faultCode</name><value><int>1</int></value>"
		 "</member>"
		 "<member>"
		 "<name>faultString</name>"
		 "<value>"
		 "<string>No such method: make.hamburger</string>"
		 "</value>"
		 "</member>"
		 "</struct>"
		 "</value>"
		 "</fault>"
		 "</methodResponse>"),
     res.toXml());
}

void XmlRpcMethodTest::testTellStatus_withoutGid()
{
  TellStatusXmlRpcMethod m;
  XmlRpcRequest req(TellStatusXmlRpcMethod::getMethodName(), BDE::list());
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

static void addUri(const std::string& uri,
		   const SharedHandle<DownloadEngine>& e)
{
  AddUriXmlRpcMethod m;
  XmlRpcRequest req(AddUriXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE::list();
  req._params[0] << BDE(uri);
  CPPUNIT_ASSERT_EQUAL(0, m.execute(req, e.get())._code);
}

#ifdef ENABLE_BITTORRENT

static void addTorrent
(const std::string& torrentFile, const SharedHandle<DownloadEngine>& e)
{
  AddTorrentXmlRpcMethod m;
  XmlRpcRequest req(AddTorrentXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE(readFile(torrentFile));
  XmlRpcResponse res = m.execute(req, e.get());
}

#endif // ENABLE_BITTORRENT

void XmlRpcMethodTest::testTellWaiting()
{
  addUri("http://1/", _e);
  addUri("http://2/", _e);
  addUri("http://3/", _e);
#ifdef ENABLE_BITTORRENT
  addTorrent("single.torrent", _e);
#endif // ENABLE_BITTORRENT

  TellWaitingXmlRpcMethod m;
  XmlRpcRequest req(TellWaitingXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE((int64_t)1);
  req._params << BDE((int64_t)2);
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(0, res._code);
  CPPUNIT_ASSERT_EQUAL((size_t)2, res._param.size());
  CPPUNIT_ASSERT_EQUAL(std::string("2"), res._param[0]["gid"].s());
  CPPUNIT_ASSERT_EQUAL(std::string("3"), res._param[1]["gid"].s());
  // waiting.size() == offset+num 
  req = XmlRpcRequest(TellWaitingXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE((int64_t)1);
#ifdef ENABLE_BITTORRENT
  req._params << BDE((int64_t)3);
#else // !ENABLE_BITTORRENT
  req._params << BDE((int64_t)2);
#endif // !ENABLE_BITTORRENT
  res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(0, res._code);
#ifdef ENABLE_BITTORRENT
  CPPUNIT_ASSERT_EQUAL((size_t)3, res._param.size());
#else // !ENABLE_BITTORRENT
  CPPUNIT_ASSERT_EQUAL((size_t)2, res._param.size());
#endif // !ENABLE_BITTORRENT
  // waiting.size() < offset+num 
  req = XmlRpcRequest(TellWaitingXmlRpcMethod::getMethodName(), BDE::list());
  req._params << BDE((int64_t)1);
  req._params << BDE((int64_t)4);
  res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(0, res._code);
#ifdef ENABLE_BITTORRENT
  CPPUNIT_ASSERT_EQUAL((size_t)3, res._param.size());
#else //!ENABLE_BITTORRENT
  CPPUNIT_ASSERT_EQUAL((size_t)2, res._param.size());
#endif // !ENABLE_BITTORRENT
}

void XmlRpcMethodTest::testTellWaiting_fail()
{
  TellWaitingXmlRpcMethod m;
  XmlRpcRequest req(TellWaitingXmlRpcMethod::getMethodName(), BDE::list());
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testGetVersion()
{
  GetVersionXmlRpcMethod m;
  XmlRpcRequest req(GetVersionXmlRpcMethod::getMethodName(), BDE::none);
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(0, res._code);
  CPPUNIT_ASSERT_EQUAL(std::string(PACKAGE_VERSION), res._param["version"].s());
  const BDE& featureList = res._param["enabledFeatures"];
  std::string features;
  for(BDE::List::const_iterator i = featureList.listBegin();
      i != featureList.listEnd(); ++i) {
    features += (*i).s();
    features += ", ";
  }
  
  CPPUNIT_ASSERT_EQUAL(FeatureConfig::getInstance()->featureSummary()+", ",
		       features);
}

void XmlRpcMethodTest::testGatherStoppedDownload()
{
  std::vector<SharedHandle<FileEntry> > fileEntries;
  std::vector<int32_t> followedBy;
  followedBy.push_back(3);
  followedBy.push_back(4);
  SharedHandle<DownloadResult> d
    (new DownloadResult(1,
			fileEntries,
			false,
			UINT64_MAX,
			1000,
			downloadresultcode::FINISHED,
			followedBy,
			2));
  BDE entry = BDE::dict();
  gatherStoppedDownload(entry, d);

  CPPUNIT_ASSERT_EQUAL(std::string("3"), entry["followedBy"][0].s());
  CPPUNIT_ASSERT_EQUAL(std::string("4"), entry["followedBy"][1].s());
  CPPUNIT_ASSERT_EQUAL(std::string("2"), entry["belongsTo"].s());
}

void XmlRpcMethodTest::testGatherProgressCommon()
{
  SharedHandle<DownloadContext> dctx(new DownloadContext());

  SharedHandle<RequestGroup> group(new RequestGroup(_option));
  group->setDownloadContext(dctx);
  std::vector<SharedHandle<RequestGroup> > followedBy;
  for(int i = 0; i < 2; ++i) {
    followedBy.push_back(SharedHandle<RequestGroup>(new RequestGroup(_option)));
  }

  group->followedBy(followedBy.begin(), followedBy.end());
  group->belongsTo(2);

  BDE entry = BDE::dict();
  gatherProgressCommon(entry, group);
  
  CPPUNIT_ASSERT_EQUAL(util::itos(followedBy[0]->getGID()),
		       entry["followedBy"][0].s());
  CPPUNIT_ASSERT_EQUAL(util::itos(followedBy[1]->getGID()),
		       entry["followedBy"][1].s());
  CPPUNIT_ASSERT_EQUAL(std::string("2"), entry["belongsTo"].s());
}

void XmlRpcMethodTest::testChangePosition()
{
  _e->_requestGroupMan->addReservedGroup
    (SharedHandle<RequestGroup>(new RequestGroup(_option)));
  _e->_requestGroupMan->addReservedGroup
    (SharedHandle<RequestGroup>(new RequestGroup(_option)));

  ChangePositionXmlRpcMethod m;
  XmlRpcRequest req(ChangePositionXmlRpcMethod::getMethodName(), BDE::list());
  req._params << std::string("1");
  req._params << BDE((int64_t)1);
  req._params << std::string("POS_SET");
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(0, res._code);
  CPPUNIT_ASSERT_EQUAL((int64_t)1, res._param.i());
  CPPUNIT_ASSERT_EQUAL
    ((int32_t)1, _e->_requestGroupMan->getReservedGroups()[1]->getGID());
}

void XmlRpcMethodTest::testChangePosition_fail()
{
  ChangePositionXmlRpcMethod m;
  XmlRpcRequest req(ChangePositionXmlRpcMethod::getMethodName(), BDE::list());
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);

  req._params << std::string("1");
  req._params << BDE((int64_t)2);
  req._params << std::string("bad keyword");
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

void XmlRpcMethodTest::testSystemMulticall()
{
  SystemMulticallXmlRpcMethod m;
  XmlRpcRequest req("system.multicall", BDE::list());
  BDE reqparams = BDE::list();
  req._params << reqparams;
  for(int i = 0; i < 2; ++i) {
    BDE dict = BDE::dict();
    dict["methodName"] = std::string(AddUriXmlRpcMethod::getMethodName());
    BDE params = BDE::list();
    params << BDE::list();
    params[0] << BDE("http://localhost/"+util::itos(i));
    dict["params"] = params;
    reqparams << dict;
  }
  {
    BDE dict = BDE::dict();
    dict["methodName"] = std::string("not exists");
    dict["params"] = BDE::list();
    reqparams << dict;
  }
  {
    reqparams << std::string("not struct");
  }
  {
    BDE dict = BDE::dict();
    dict["methodName"] = std::string("system.multicall");
    dict["params"] = BDE::list();
    reqparams << dict;
  }
  {
    // missing params
    BDE dict = BDE::dict();
    dict["methodName"] = std::string(GetVersionXmlRpcMethod::getMethodName());
    reqparams << dict;
  }
  {
    BDE dict = BDE::dict();
    dict["methodName"] = std::string(GetVersionXmlRpcMethod::getMethodName());
    dict["params"] = BDE::list();
    reqparams << dict;
  }
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(0, res._code);
  CPPUNIT_ASSERT_EQUAL((size_t)7, res._param.size());
  CPPUNIT_ASSERT_EQUAL(std::string("1"), res._param[0][0].s());
  CPPUNIT_ASSERT_EQUAL(std::string("2"), res._param[1][0].s());
  CPPUNIT_ASSERT_EQUAL((int64_t)1, res._param[2]["faultCode"].i());
  CPPUNIT_ASSERT_EQUAL((int64_t)1, res._param[3]["faultCode"].i());
  CPPUNIT_ASSERT_EQUAL((int64_t)1, res._param[4]["faultCode"].i());
  CPPUNIT_ASSERT_EQUAL((int64_t)1, res._param[5]["faultCode"].i());
  CPPUNIT_ASSERT(res._param[6].isList());
}

void XmlRpcMethodTest::testSystemMulticall_fail()
{
  SystemMulticallXmlRpcMethod m;
  XmlRpcRequest req("system.multicall", BDE::list());
  XmlRpcResponse res = m.execute(req, _e.get());
  CPPUNIT_ASSERT_EQUAL(1, res._code);
}

} // namespace xmlrpc

} // namespace aria2
