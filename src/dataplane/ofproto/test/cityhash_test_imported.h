#ifndef __SRC_DATAPLANE_OFPROTO_TEST_CITYHASH_TEST_IMPORTED_H__
#define __SRC_DATAPLANE_OFPROTO_TEST_CITYHASH_TEST_IMPORTED_H__

/*-
 * This header file has been derived from src/city-test.cc in CityHash
 * version 1.0.3 by Google, Inc.
 *
 * Original copyright statement:
 */
// Copyright (c) 2011 Google, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

/*
 * Compatibility pads.
 */

#define KSEED128(_a, _b)                        \
  {                                             \
    .first = (_a),                              \
             .second = (_b),                            \
  }

/*
 * Declarations.
 */
#define K0              (0xc3a5c85c97cb3127ULL)
#define KSEED0          (1234567)
#define KSEED1          (K0)
#define KDATASIZE       (1 << 20)
#define KTESTSIZE       (300)
static const uint64 k0 = K0;
static const uint64 kSeed0 = KSEED0;
static const uint64 kSeed1 = K0;
static const uint128 kSeed128 = KSEED128(KSEED0, KSEED1);
static const int kDataSize = KDATASIZE;
static const int kTestSize = KTESTSIZE;

static char data[KDATASIZE];

static int errors = 0;  // global error count


/*
 * Expected results.
 */
#define C(x) 0x ## x ## ULL
static const uint64 testdata[KTESTSIZE][15] = {
  {
    C(9ae16a3b2f90404f), C(75106db890237a4a), C(3feac5f636039766), C(3df09dfc64c09a2b), C(3cb540c392e51e29), C(6b56343feac0663), C(5b7bc50fd8e8ad92),
    C(3df09dfc64c09a2b), C(3cb540c392e51e29), C(6b56343feac0663), C(5b7bc50fd8e8ad92),
    C(889f555a0f5b2dc0), C(7767800902c8a8ce), C(bcd2a808f4cb4a44), C(e9024dba8f94f2f3)
  },
  {
    C(75e9dee28ded761d), C(931992c1b14334c5), C(245eeb25ba2c172e), C(1290f0e8a5caa74d), C(ca4c6bf7583f5cda), C(e1d60d51632c536d), C(cbc54a1db641910a),
    C(1290f0e8a5caa74d), C(ca4c6bf7583f5cda), C(e1d60d51632c536d), C(cbc54a1db641910a),
    C(9866d68d17c2c08e), C(8d84ba63eb4d020a), C(df0ad99c78cbce44), C(7c98593ef62573ed)
  },
  {
    C(75de892fdc5ba914), C(f89832e71f764c86), C(39a82df1f278a297), C(b4af8ae673acb930), C(992b7acb203d8885), C(57b533f3f8b94d50), C(bbb69298a5dcf1a1),
    C(b4af8ae673acb930), C(992b7acb203d8885), C(57b533f3f8b94d50), C(bbb69298a5dcf1a1),
    C(433495196af9ac4f), C(53445c0896ae1fe6), C(f7b939315f6fb56f), C(ac1b05e5a2e0335e)
  },
  {
    C(69cfe9fca1cc683a), C(e65f2a81e19b8067), C(20575ea6370a9d14), C(8f52532fc6f005b7), C(4ebe60df371ec129), C(c6ef8a7f8deb8116), C(83df17e3c9bb9a67),
    C(8f52532fc6f005b7), C(4ebe60df371ec129), C(c6ef8a7f8deb8116), C(83df17e3c9bb9a67),
    C(6a0aaf51016e19cd), C(fb0d1e89f39dbf6a), C(c73095102872943a), C(405ea97456c28a75)
  },
  {
    C(675b04c582a34966), C(53624b5ef8cd4f45), C(c412e0931ac8c9b1), C(798637e677c65a3), C(83e3b06adc4cd3ff), C(f3e76e8a7135852f), C(111e66cfbb05366d),
    C(798637e677c65a3), C(83e3b06adc4cd3ff), C(f3e76e8a7135852f), C(111e66cfbb05366d),
    C(29c4f84aa48e8682), C(b77a8685750c94d0), C(7cab65571969123f), C(fb1dbd79f68a8519)
  },
  {
    C(46fa817397ea8b68), C(cc960c1c15ce2d20), C(e5f9f947bafb9e79), C(b342cdf0d7ac4b2a), C(66914d44b373b232), C(261194e76cb43966), C(45a0010190365048),
    C(b342cdf0d7ac4b2a), C(66914d44b373b232), C(261194e76cb43966), C(45a0010190365048),
    C(e2586947ca8eac83), C(6650daf2d9677cdc), C(2f9533d8f4951a9), C(a5bdc0f3edc4bd7b)
  },
  {
    C(406e959cdffadec7), C(e80dc125dca28ed1), C(e5beb146d4b79a21), C(e66d5c1bb441541a), C(d14961bc1fd265a2), C(e4cc669d4fc0577f), C(abf4a51e36da2702),
    C(e66d5c1bb441541a), C(d14961bc1fd265a2), C(e4cc669d4fc0577f), C(abf4a51e36da2702),
    C(21236d12df338f75), C(54b8c4a5ad2ae4a4), C(202d50ef9c2d4465), C(5ecc6a128e51a797)
  },
  {
    C(46663908b4169b95), C(4e7e90b5c426bf1d), C(dc660b58daaf8b2c), C(b298265ebd1bd55f), C(4a5f6838b55c0b08), C(fc003c97aa05d397), C(2fb5adad3380c3bc),
    C(b298265ebd1bd55f), C(4a5f6838b55c0b08), C(fc003c97aa05d397), C(2fb5adad3380c3bc),
    C(c46fd01d253b4a0b), C(4c799235c2a33188), C(7e21bc57487a11bf), C(e1392bb1994bd4f2)
  },
  {
    C(f214b86cffeab596), C(5fccb0b132da564f), C(86e7aa8b4154b883), C(763529c8d4189ea8), C(860d77e7fef74ca3), C(3b1ba41191219b6b), C(722b25dfa6d0a04b),
    C(763529c8d4189ea8), C(860d77e7fef74ca3), C(3b1ba41191219b6b), C(722b25dfa6d0a04b),
    C(5f7b463094e22a91), C(75d6f57376b31bd7), C(d253c7f89efec8e6), C(efe56ac880a2b8a3)
  },
  {
    C(eba670441d1a4f7d), C(eb6b272502d975fa), C(69f8d424d50c083e), C(313d49cb51b8cd2c), C(6e982d8b4658654a), C(dd59629a17e5492d), C(81cb23bdab95e30e),
    C(313d49cb51b8cd2c), C(6e982d8b4658654a), C(dd59629a17e5492d), C(81cb23bdab95e30e),
    C(1e6c3e6c454c774f), C(177655172666d5ea), C(9cc67e0d38d80886), C(36a2d64d7bc58d22)
  },
  {
    C(172c17ff21dbf88d), C(1f5104e320f0c815), C(1e34e9f1fa63bcef), C(3506ae8fae368d2a), C(59fa2b2de5306203), C(67d1119dcfa6007e), C(1f7190c648ad9aef),
    C(3506ae8fae368d2a), C(59fa2b2de5306203), C(67d1119dcfa6007e), C(1f7190c648ad9aef),
    C(7e8b1e689137b637), C(cbe373368a31db3c), C(dbc79d82cd49c671), C(641399520c452c99)
  },
  {
    C(5a0838df8a019b8c), C(73fc859b4952923), C(45e39daf153491bd), C(a9b91459a5fada46), C(de0fbf8800a2da3), C(21800e4b5af9dedb), C(517c3726ae0dbae7),
    C(a9b91459a5fada46), C(de0fbf8800a2da3), C(21800e4b5af9dedb), C(517c3726ae0dbae7),
    C(1ccffbd74acf9d66), C(cbb08cf95e7eda99), C(61444f09e2a29587), C(35c0d15745f96455)
  },
  {
    C(8f42b1fbb2fc0302), C(5ae31626076ab6ca), C(b87f0cb67cb75d28), C(2498586ac2e1fab2), C(e683f9cbea22809a), C(a9728d0b2bbe377c), C(46baf5cae53dc39a),
    C(2498586ac2e1fab2), C(e683f9cbea22809a), C(a9728d0b2bbe377c), C(46baf5cae53dc39a),
    C(806f4352c99229e), C(d4643728fc71754a), C(998c1647976bc893), C(d8094fdc2d6bb032)
  },
  {
    C(72085e82d70dcea9), C(32f502c43349ba16), C(5ebc98c3645a018f), C(c7fa762238fd90ac), C(8d03b5652d615677), C(a3f5226e51d42217), C(46d5010a7cae8c1e),
    C(c7fa762238fd90ac), C(8d03b5652d615677), C(a3f5226e51d42217), C(46d5010a7cae8c1e),
    C(4293122580db7f5f), C(3df6042f39c6d487), C(439124809cf5c90e), C(90b704e4f71d0ccf)
  },
  {
    C(32b75fc2223b5032), C(246fff80eb230868), C(a6fdbc82c9aeecc0), C(c089498074167021), C(ab094a9f9ab81c23), C(4facf3d9466bcb03), C(57aa9c67938cf3eb),
    C(c089498074167021), C(ab094a9f9ab81c23), C(4facf3d9466bcb03), C(57aa9c67938cf3eb),
    C(79a769ca1c762117), C(9c8dee60337f87a8), C(dabf1b96535a3abb), C(f87e9fbb590ba446)
  },
  {
    C(e1dd010487d2d647), C(12352858295d2167), C(acc5e9b6f6b02dbb), C(1c66ceea473413df), C(dc3f70a124b25a40), C(66a6dfe54c441cd8), C(b436dabdaaa37121),
    C(1c66ceea473413df), C(dc3f70a124b25a40), C(66a6dfe54c441cd8), C(b436dabdaaa37121),
    C(6d95aa6890f51674), C(42c6c0fc7ab3c107), C(83b9dfe082e76140), C(939cdbd3614d6416)
  },
  {
    C(2994f9245194a7e2), C(b7cd7249d6db6c0c), C(2170a7d119c5c6c3), C(8505c996b70ee9fc), C(b92bba6b5d778eb7), C(4db4c57f3a7a4aee), C(3cfd441cb222d06f),
    C(8505c996b70ee9fc), C(b92bba6b5d778eb7), C(4db4c57f3a7a4aee), C(3cfd441cb222d06f),
    C(4d940313c96ac6bd), C(43762837c9ffac4b), C(480fcf58920722e3), C(4bbd1e1a1d06752f)
  },
  {
    C(32e2ed6fa03e5b22), C(58baf09d7c71c62b), C(a9c599f3f8f50b5b), C(1660a2c4972d0fa1), C(1a1538d6b50a57c), C(8a5362485bbc9363), C(e8eec3c84fd9f2f8),
    C(1660a2c4972d0fa1), C(1a1538d6b50a57c), C(8a5362485bbc9363), C(e8eec3c84fd9f2f8),
    C(2562514461d373da), C(33857675fed52b4), C(e58d2a17057f1943), C(fe7d3f30820e4925)
  },
  {
    C(37a72b6e89410c9f), C(139fec53b78cee23), C(4fccd8f0da7575c3), C(3a5f04166518ac75), C(f49afe05a44fc090), C(cb01b4713cfda4bd), C(9027bd37ffc0a5de),
    C(3a5f04166518ac75), C(f49afe05a44fc090), C(cb01b4713cfda4bd), C(9027bd37ffc0a5de),
    C(e15144d3ad46ec1b), C(736fd99679a5ae78), C(b3d7ed9ed0ddfe57), C(cef60639457867d7)
  },
  {
    C(10836563cb8ff3a1), C(d36f67e2dfc085f7), C(edc1bb6a3dcba8df), C(bd4f3a0566df3bed), C(81fc8230c163dcbe), C(4168bc8417a8281b), C(7100c9459827c6a6),
    C(bd4f3a0566df3bed), C(81fc8230c163dcbe), C(4168bc8417a8281b), C(7100c9459827c6a6),
    C(21cad59eaf79e72f), C(61c8af6fb71469f3), C(b0dfc42ce4f578b), C(33ea34ccea305d4e)
  },
  {
    C(4dabcb5c1d382e5c), C(9a868c608088b7a4), C(7b2b6c389b943be5), C(c914b925ab69fda0), C(6bafe864647c94d7), C(7a48682dd4afa22), C(40fe01210176ba10),
    C(c914b925ab69fda0), C(6bafe864647c94d7), C(7a48682dd4afa22), C(40fe01210176ba10),
    C(88dd28f33ec31388), C(c6db60abf1d45381), C(7b94c447298824d5), C(6b2a5e05ad0b9fc0)
  },
  {
    C(296afb509046d945), C(c38fe9eb796bd4be), C(d7b17535df110279), C(dd2482b87d1ade07), C(662785d2e3e78ddf), C(eae39994375181bb), C(9994500c077ee1db),
    C(dd2482b87d1ade07), C(662785d2e3e78ddf), C(eae39994375181bb), C(9994500c077ee1db),
    C(a275489f8c6bb289), C(30695ea31df1a369), C(1aeeb31802d701b5), C(7799d5a6d5632838)
  },
  {
    C(f7c0257efde772ea), C(af6af9977ecf7bff), C(1cdff4bd07e8d973), C(fab1f4acd2cd4ab4), C(b4e19ba52b566bd), C(7f1db45725fe2881), C(70276ff8763f8396),
    C(fab1f4acd2cd4ab4), C(b4e19ba52b566bd), C(7f1db45725fe2881), C(70276ff8763f8396),
    C(1b0f2b546dddd16b), C(aa066984b5fd5144), C(7c3f9386c596a5a8), C(e5befdb24b665d5f)
  },
  {
    C(61e021c8da344ba1), C(cf9c720676244755), C(354ffa8e9d3601f6), C(44e40a03093fbd92), C(bda9481cc5b93cae), C(986b589cbc0cf617), C(210f59f074044831),
    C(44e40a03093fbd92), C(bda9481cc5b93cae), C(986b589cbc0cf617), C(210f59f074044831),
    C(ac32cbbb6f50245a), C(afa6f712efb22075), C(47289f7af581719f), C(31b6e75d3aa0e54b)
  },
  {
    C(c0a86ed83908560b), C(440c8b6f97bd1749), C(a99bf2891726ea93), C(ac0c0b84df66df9d), C(3ee2337b437eb264), C(8a341daed9a25f98), C(cc665499aa38c78c),
    C(ac0c0b84df66df9d), C(3ee2337b437eb264), C(8a341daed9a25f98), C(cc665499aa38c78c),
    C(af7275299d79a727), C(874fa8434b45d0e), C(ca7b67388950dd33), C(2db5cd3675ec58f7)
  },
  {
    C(35c9cf87e4accbf3), C(2267eb4d2191b2a3), C(80217695666b2c9), C(cd43a24abbaae6d), C(a88abf0ea1b2a8ff), C(e297ff01427e2a9d), C(935d545695b2b41d),
    C(cd43a24abbaae6d), C(a88abf0ea1b2a8ff), C(e297ff01427e2a9d), C(935d545695b2b41d),
    C(6accd4dbcb52e849), C(261295acb662fd49), C(f9d91f1ac269a8a2), C(5e45f39df355e395)
  },
  {
    C(e74c366b3091e275), C(522e657c5da94b06), C(ca9afa806f1a54ac), C(b545042f67929471), C(90d10e75ed0e75d8), C(3ea60f8f158df77e), C(8863eff3c2d670b7),
    C(b545042f67929471), C(90d10e75ed0e75d8), C(3ea60f8f158df77e), C(8863eff3c2d670b7),
    C(5799296e97f144a7), C(1d6e517c12a88271), C(da579e9e1add90ef), C(942fb4cdbc3a4da)
  },
  {
    C(a3f2ca45089ad1a6), C(13f6270fe56fbce4), C(1f93a534bf03e705), C(aaea14288ae2d90c), C(1be3cd51ef0f15e8), C(e8b47c84d5a4aac1), C(297d27d55b766782),
    C(aaea14288ae2d90c), C(1be3cd51ef0f15e8), C(e8b47c84d5a4aac1), C(297d27d55b766782),
    C(e922d1d8bb2afd0), C(b4481c4fa2e7d8d5), C(691e21538af794d5), C(9bd4fb0a53962a72)
  },
  {
    C(e5181466d8e60e26), C(cf31f3a2d582c4f3), C(d9cee87cb71f75b2), C(4750ca6050a2d726), C(d6e6dd8940256849), C(f3b3749fdab75b0), C(c55d8a0f85ba0ccf),
    C(4750ca6050a2d726), C(d6e6dd8940256849), C(f3b3749fdab75b0), C(c55d8a0f85ba0ccf),
    C(47f134f9544c6da6), C(e1cdd9cb74ad764), C(3ce2d096d844941e), C(321fe62f608d2d4e)
  },
  {
    C(fb528a8dd1e48ad7), C(98c4fd149c8a63dd), C(4abd8fc3377ae1f), C(d7a9304abbb47cc5), C(7f2b9a27aa57f99), C(353ab332d4ef9f18), C(47d56b8d6c8cf578),
    C(d7a9304abbb47cc5), C(7f2b9a27aa57f99), C(353ab332d4ef9f18), C(47d56b8d6c8cf578),
    C(df55f58ae09f311f), C(dba9511784fa86e3), C(c43ce0288858a47e), C(62971e94270b78e1)
  },
  {
    C(da6d2b7ea9d5f9b6), C(57b11153ee3b4cc8), C(7d3bd1256037142f), C(90b16ff331b719b5), C(fc294e7ad39e01e6), C(d2145386bab41623), C(7045a63d44d76011),
    C(90b16ff331b719b5), C(fc294e7ad39e01e6), C(d2145386bab41623), C(7045a63d44d76011),
    C(a232222ed0fe2fa4), C(e6c17dff6c323a8a), C(bbcb079be123ac6c), C(4121fe2c5de7d850)
  },
  {
    C(61d95225bc2293e), C(f6c52cb6be9889a8), C(91a0667a7ed6a113), C(441133d221486a3d), C(fb9c5a40e19515b), C(6c967b6c69367c2d), C(145bd9ef258c4099),
    C(441133d221486a3d), C(fb9c5a40e19515b), C(6c967b6c69367c2d), C(145bd9ef258c4099),
    C(a0197657160c686e), C(91ada0871c23f379), C(2fd74fceccb5c80c), C(bf04f24e2dc17913)
  },
  {
    C(81247c01ab6a9cc1), C(fbccea953e810636), C(ae18965000c31be0), C(15bb46383daec2a5), C(716294063b4ba089), C(f3bd691ce02c3014), C(14ccaad685a20764),
    C(15bb46383daec2a5), C(716294063b4ba089), C(f3bd691ce02c3014), C(14ccaad685a20764),
    C(5db25914279d6f24), C(25c451fce3b2ed06), C(e6bacb43ba1ddb9a), C(6d77493a2e6fd76)
  },
  {
    C(c17f3ebd3257cb8b), C(e9e68c939c118c8d), C(72a5572be35bfc1b), C(f6916c341cb31f2a), C(591da1353ee5f31c), C(f1313c98a836b407), C(e0b8473eada48cd1),
    C(f6916c341cb31f2a), C(591da1353ee5f31c), C(f1313c98a836b407), C(e0b8473eada48cd1),
    C(ac5c2fb40b09ba46), C(3a3e8a9344eb6548), C(3bf9349a9d8483a6), C(c30dd0d9b15e92d0)
  },
  {
    C(9802438969c3043b), C(6cd07575c948dd82), C(83e26b6830ea8640), C(d52f1fa190576961), C(11d182e4f0d419cc), C(5d9ccf1b56617424), C(c8a16debb585e452),
    C(d52f1fa190576961), C(11d182e4f0d419cc), C(5d9ccf1b56617424), C(c8a16debb585e452),
    C(2158a752d2686d40), C(b93c1fdf54789e8c), C(3a9a435627b2a30b), C(de6e5e551e7e5ad5)
  },
  {
    C(3dd8ed248a03d754), C(d8c1fcf001cb62e0), C(87a822141ed64927), C(4bfaf6fd26271f47), C(aefeae8222ad3c77), C(cfb7b24351a60585), C(8678904e9e890b8f),
    C(4bfaf6fd26271f47), C(aefeae8222ad3c77), C(cfb7b24351a60585), C(8678904e9e890b8f),
    C(968dd1aa4d7dcf31), C(7ac643b015007a39), C(d1e1bac3d94406ec), C(babfa52474a404fa)
  },
  {
    C(c5bf48d7d3e9a5a3), C(8f0249b5c5996341), C(c6d2c8a606f45125), C(fd1779db740e2c48), C(1950ef50fefab3f8), C(e4536426a6196809), C(699556c502a01a6a),
    C(fd1779db740e2c48), C(1950ef50fefab3f8), C(e4536426a6196809), C(699556c502a01a6a),
    C(2f49d268bb57b946), C(b205baa6c66ebfa5), C(ab91ebe4f48b0da1), C(c7e0718ccc360328)
  },
  {
    C(bc4a21d00cf52288), C(28df3eb5a533fa87), C(6081bbc2a18dd0d), C(8eed355d219e58b9), C(2d7b9f1a3d645165), C(5758d1aa8d85f7b2), C(9c90c65920041dff),
    C(8eed355d219e58b9), C(2d7b9f1a3d645165), C(5758d1aa8d85f7b2), C(9c90c65920041dff),
    C(3c5c4ea46645c7f1), C(346879ecc0e2eb90), C(8434fec461bb5a0f), C(783ccede50ef5ce9)
  },
  {
    C(172c8674913ff413), C(1815a22400e832bf), C(7e011f9467a06650), C(161be43353a31dd0), C(79a8afddb0642ac3), C(df43af54e3e16709), C(6e12553a75b43f07),
    C(161be43353a31dd0), C(79a8afddb0642ac3), C(df43af54e3e16709), C(6e12553a75b43f07),
    C(3ac1b1121e87d023), C(2d47d33df7b9b027), C(e2d3f71f4e817ff5), C(70b3a11ca85f8a39)
  },
  {
    C(17a361dbdaaa7294), C(c67d368223a3b83c), C(f49cf8d51ab583d2), C(666eb21e2eaa596), C(778f3e1b6650d56), C(3f6be451a668fe2d), C(5452892b0b101388),
    C(666eb21e2eaa596), C(778f3e1b6650d56), C(3f6be451a668fe2d), C(5452892b0b101388),
    C(cc867fceaeabdb95), C(f238913c18aaa101), C(f5236b44f324cea1), C(c507cc892ff83dd1)
  },
  {
    C(5cc268bac4bd55f), C(232717a35d5b2f1), C(38da1393365c961d), C(2d187f89c16f7b62), C(4eb504204fa1be8), C(222bd53d2efe5fa), C(a4dcd6d721ddb187),
    C(2d187f89c16f7b62), C(4eb504204fa1be8), C(222bd53d2efe5fa), C(a4dcd6d721ddb187),
    C(d86bbe67666eca70), C(c8bbae99d8e6429f), C(41dac4ceb2cb6b10), C(2f90c331755f6c48)
  },
  {
    C(db04969cc06547f1), C(fcacc8a75332f120), C(967ccec4ed0c977e), C(ac5d1087e454b6cd), C(c1f8b2e284d28f6c), C(cc3994f4a9312cfa), C(8d61606dbc4e060d),
    C(ac5d1087e454b6cd), C(c1f8b2e284d28f6c), C(cc3994f4a9312cfa), C(8d61606dbc4e060d),
    C(17315af3202a1307), C(850775145e01163a), C(96f10e7357f930d2), C(abf27049cf07129)
  },
  {
    C(25bd8d3ca1b375b2), C(4ad34c2c865816f9), C(9be30ad32f8f28aa), C(7755ea02dbccad6a), C(cb8aaf8886247a4a), C(8f6966ce7ea1b6e6), C(3f2863090fa45a70),
    C(7755ea02dbccad6a), C(cb8aaf8886247a4a), C(8f6966ce7ea1b6e6), C(3f2863090fa45a70),
    C(1e46d73019c9fb06), C(af37f39351616f2c), C(657efdfff20ea2ed), C(93bdf4c58ada3ecb)
  },
  {
    C(166c11fbcbc89fd8), C(cce1af56c48a48aa), C(78908959b8ede084), C(19032925ba2c951a), C(a53ed6e81b67943a), C(edc871a9e8ef4bdf), C(ae66cf46a8371aba),
    C(19032925ba2c951a), C(a53ed6e81b67943a), C(edc871a9e8ef4bdf), C(ae66cf46a8371aba),
    C(a37b97790fe75861), C(eda28c8622708b98), C(3f0a23509d3d5c9d), C(5787b0e7976c97cf)
  },
  {
    C(3565bcc4ca4ce807), C(ec35bfbe575819d5), C(6a1f690d886e0270), C(1ab8c584625f6a04), C(ccfcdafb81b572c4), C(53b04ba39fef5af9), C(64ce81828eefeed4),
    C(1ab8c584625f6a04), C(ccfcdafb81b572c4), C(53b04ba39fef5af9), C(64ce81828eefeed4),
    C(131af99997fc662c), C(8d9081192fae833c), C(2828064791cb2eb), C(80554d2e8294065c)
  },
  {
    C(b7897fd2f274307d), C(6d43a9e5dd95616d), C(31a2218e64d8fce0), C(664e581fc1cf769b), C(415110942fc97022), C(7a5d38fee0bfa763), C(dc87ddb4d7495b6c),
    C(664e581fc1cf769b), C(415110942fc97022), C(7a5d38fee0bfa763), C(dc87ddb4d7495b6c),
    C(7c3b66372e82e64b), C(1c89c0ceeeb2dd1), C(dad76d2266214dbd), C(744783486e43cc61)
  },
  {
    C(aba98113ab0e4a16), C(287f883aede0274d), C(3ecd2a607193ba3b), C(e131f6cc9e885c28), C(b399f98d827e4958), C(6eb90c8ed6c9090c), C(ec89b378612a2b86),
    C(e131f6cc9e885c28), C(b399f98d827e4958), C(6eb90c8ed6c9090c), C(ec89b378612a2b86),
    C(cfc0e126e2f860c0), C(a9a8ab5dec95b1c), C(d06747f372f7c733), C(fbd643f943a026d3)
  },
  {
    C(17f7796e0d4b636c), C(ddba5551d716137b), C(65f9735375df1ada), C(a39e946d02e14ec2), C(1c88cc1d3822a193), C(663f8074a5172bb4), C(8ad2934942e4cb9c),
    C(a39e946d02e14ec2), C(1c88cc1d3822a193), C(663f8074a5172bb4), C(8ad2934942e4cb9c),
    C(3da03b033a95f16c), C(54a52f1932a1749d), C(779eeb734199bc25), C(359ce8c8faccc57b)
  },
  {
    C(33c0128e62122440), C(b23a588c8c37ec2b), C(f2608199ca14c26a), C(acab0139dc4f36df), C(9502b1605ca1345a), C(32174ef1e06a5e9c), C(d824b7869258192b),
    C(acab0139dc4f36df), C(9502b1605ca1345a), C(32174ef1e06a5e9c), C(d824b7869258192b),
    C(681d021b52064762), C(30b6c735f80ac371), C(6a12d8d7f78896b3), C(157111657a972144)
  },
  {
    C(988bc5d290b97aef), C(6754bb647eb47666), C(44b5cf8b5b8106a8), C(a1c5ba961937f723), C(32d6bc7214dfcb9b), C(6863397e0f4c6758), C(e644bcb87e3eef70),
    C(a1c5ba961937f723), C(32d6bc7214dfcb9b), C(6863397e0f4c6758), C(e644bcb87e3eef70),
    C(bf25ae22e7aa7c97), C(f5f3177da5756312), C(56a469cb0dbb58cd), C(5233184bb6130470)
  },
  {
    C(23c8c25c2ab72381), C(d6bc672da4175fba), C(6aef5e6eb4a4eb10), C(3df880c945e68aed), C(5e08a75e956d456f), C(f984f088d1a322d7), C(7d44a1b597b7a05e),
    C(3df880c945e68aed), C(5e08a75e956d456f), C(f984f088d1a322d7), C(7d44a1b597b7a05e),
    C(cbd7d157b7fcb020), C(2e2945e90749c2aa), C(a86a13c934d8b1bb), C(fbe3284bb4eab95f)
  },
  {
    C(450fe4acc4ad3749), C(3111b29565e4f852), C(db570fc2abaf13a9), C(35107d593ba38b22), C(fd8212a125073d88), C(72805d6e015bfacf), C(6b22ae1a29c4b853),
    C(35107d593ba38b22), C(fd8212a125073d88), C(72805d6e015bfacf), C(6b22ae1a29c4b853),
    C(df2401f5c3c1b633), C(c72307e054c81c8f), C(3efbfe65bd2922c0), C(b4f632e240b3190c)
  },
  {
    C(48e1eff032d90c50), C(dee0fe333d962b62), C(c845776990c96775), C(8ea71758346b71c9), C(d84258cab79431fd), C(af566b4975cce10a), C(5c5c7e70a91221d2),
    C(8ea71758346b71c9), C(d84258cab79431fd), C(af566b4975cce10a), C(5c5c7e70a91221d2),
    C(c33202c7be49ea6f), C(e8ade53b6cbf4caf), C(102ea04fc82ce320), C(c1f7226614715e5e)
  },
  {
    C(c048604ba8b6c753), C(21ea6d24b417fdb6), C(4e40a127ad2d6834), C(5234231bf173c51), C(62319525583eaf29), C(87632efa9144cc04), C(1749de70c8189067),
    C(5234231bf173c51), C(62319525583eaf29), C(87632efa9144cc04), C(1749de70c8189067),
    C(29672240923e8207), C(11dd247a815f6d0d), C(8d64e16922487ed0), C(9fa6f45d50d83627)
  },
  {
    C(67ff1cbe469ebf84), C(3a828ac9e5040eb0), C(85bf1ad6b363a14b), C(2fc6c0783390d035), C(ef78307f5be5524e), C(a46925b7a1a77905), C(fea37470f9a51514),
    C(2fc6c0783390d035), C(ef78307f5be5524e), C(a46925b7a1a77905), C(fea37470f9a51514),
    C(9d6504cf6d3947ce), C(174cc006b8e96e7), C(d653a06d8a009836), C(7d22b5399326a76c)
  },
  {
    C(b45c7536bd7a5416), C(e2d17c16c4300d3c), C(b70b641138765ff5), C(a5a859ab7d0ddcfc), C(8730164a0b671151), C(af93810c10348dd0), C(7256010c74f5d573),
    C(a5a859ab7d0ddcfc), C(8730164a0b671151), C(af93810c10348dd0), C(7256010c74f5d573),
    C(e22a335be6cd49f3), C(3bc9c8b40c9c397a), C(18da5c08e28d3fb5), C(f58ea5a00404a5c9)
  },
  {
    C(215c2eaacdb48f6f), C(33b09acf1bfa2880), C(78c4e94ba9f28bf), C(981b7219224443d1), C(1f476fc4344d7bba), C(abad36e07283d3a5), C(831bf61190eaaead),
    C(981b7219224443d1), C(1f476fc4344d7bba), C(abad36e07283d3a5), C(831bf61190eaaead),
    C(4c90729f62432254), C(2ffadc94c89f47b3), C(677e790b43d20e9a), C(bb0a1686e7c3ae5f)
  },
  {
    C(241baf16d80e0fe8), C(b6b3c5b53a3ce1d), C(6ae6b36209eecd70), C(a560b6a4aa3743a4), C(b3e04f202b7a99b), C(3b3b1573f4c97d9f), C(ccad8715a65af186),
    C(a560b6a4aa3743a4), C(b3e04f202b7a99b), C(3b3b1573f4c97d9f), C(ccad8715a65af186),
    C(d0c93a838b0c37e7), C(7150aa1be7eb1aad), C(755b1e60b84d8d), C(51916e77b1b05ba9)
  },
  {
    C(d10a9743b5b1c4d1), C(f16e0e147ff9ccd6), C(fbd20a91b6085ed3), C(43d309eb00b771d5), C(a6d1f26105c0f61b), C(d37ad62406e5c37e), C(75d9b28c717c8cf7),
    C(43d309eb00b771d5), C(a6d1f26105c0f61b), C(d37ad62406e5c37e), C(75d9b28c717c8cf7),
    C(8f5f118b425b57cd), C(5d806318613275f3), C(8150848bcf89d009), C(d5531710d53e1462)
  },
  {
    C(919ef9e209f2edd1), C(684c33fb726a720a), C(540353f94e8033), C(26da1a143e7d4ec4), C(55095eae445aacf4), C(31efad866d075938), C(f9b580cff4445f94),
    C(26da1a143e7d4ec4), C(55095eae445aacf4), C(31efad866d075938), C(f9b580cff4445f94),
    C(b1bea6b8716d9c48), C(9ed2a3df4a15dc53), C(11f1be58843eb8e9), C(d9899ecaaef3c77c)
  },
  {
    C(b5f9519b6c9280b), C(7823a2fe2e103803), C(d379a205a3bd4660), C(466ec55ee4b4302a), C(714f1b9985deeaf0), C(728595f26e633cf7), C(25ecd0738e1bee2b),
    C(466ec55ee4b4302a), C(714f1b9985deeaf0), C(728595f26e633cf7), C(25ecd0738e1bee2b),
    C(db51771ad4778278), C(763e5742ac55639e), C(df040e92d38aa785), C(5df997d298499bf1)
  },
  {
    C(77a75e89679e6757), C(25d31fee616b5dd0), C(d81f2dfd08890060), C(7598df8911dd40a4), C(3b6dda517509b41b), C(7dae29d248dfffae), C(6697c427733135f),
    C(7598df8911dd40a4), C(3b6dda517509b41b), C(7dae29d248dfffae), C(6697c427733135f),
    C(834d6c0444c90899), C(c790675b3cd53818), C(28bb4c996ecadf18), C(92c648513e6e6064)
  },
  {
    C(9d709e1b086aabe2), C(4d6d6a6c543e3fec), C(df73b01acd416e84), C(d54f613658e35418), C(fcc88fd0567afe77), C(d18f2380980db355), C(ec3896137dfbfa8b),
    C(d54f613658e35418), C(fcc88fd0567afe77), C(d18f2380980db355), C(ec3896137dfbfa8b),
    C(eb48dbd9a1881600), C(ca7bd7415ab43ca9), C(e6c5a362919e2351), C(2f4e4bd2d5267c21)
  },
  {
    C(91c89971b3c20a8a), C(87b82b1d55780b5), C(bc47bb80dfdaefcd), C(87e11c0f44454863), C(2df1aedb5871cc4b), C(ba72fd91536382c8), C(52cebef9e6ea865d),
    C(87e11c0f44454863), C(2df1aedb5871cc4b), C(ba72fd91536382c8), C(52cebef9e6ea865d),
    C(5befc3fc66bc7fc5), C(b128bbd735a89061), C(f8f500816fa012b3), C(f828626c9612f04)
  },
  {
    C(16468c55a1b3f2b4), C(40b1e8d6c63c9ff4), C(143adc6fee592576), C(4caf4deeda66a6ee), C(264720f6f35f7840), C(71c3aef9e59e4452), C(97886ca1cb073c55),
    C(4caf4deeda66a6ee), C(264720f6f35f7840), C(71c3aef9e59e4452), C(97886ca1cb073c55),
    C(16155fef16fc08e8), C(9d0c1d1d5254139a), C(246513bf2ac95ee2), C(22c8440f59925034)
  },
  {
    C(1a2bd6641870b0e4), C(e4126e928f4a7314), C(1e9227d52aab00b2), C(d82489179f16d4e8), C(a3c59f65e2913cc5), C(36cbaecdc3532b3b), C(f1b454616cfeca41),
    C(d82489179f16d4e8), C(a3c59f65e2913cc5), C(36cbaecdc3532b3b), C(f1b454616cfeca41),
    C(99393e31e3eefc16), C(3ca886eac5754cdf), C(c11776fc3e4756dd), C(ca118f7059198ba)
  },
  {
    C(1d2f92f23d3e811a), C(e0812edbcd475412), C(92d2d6ad29c05767), C(fd7feb3d2956875e), C(d7192a886b8b01b6), C(16e71dba55f5b85a), C(93dabd3ff22ff144),
    C(fd7feb3d2956875e), C(d7192a886b8b01b6), C(16e71dba55f5b85a), C(93dabd3ff22ff144),
    C(14ff0a5444c272c9), C(fb024d3bb8d915c2), C(1bc3229a94cab5fe), C(6f6f1fb3c0dccf09)
  },
  {
    C(a47c08255da30ca8), C(cf6962b7353f4e68), C(2808051ea18946b1), C(b5b472960ece11ec), C(13935c99b9abbf53), C(3e80d95687f0432c), C(3516ab536053be5),
    C(b5b472960ece11ec), C(13935c99b9abbf53), C(3e80d95687f0432c), C(3516ab536053be5),
    C(748ce6a935755e20), C(2961b51d61b0448c), C(864624113aae88d2), C(a143805366f91338)
  },
  {
    C(efb3b0262c9cd0c), C(1273901e9e7699b3), C(58633f4ad0dcd5bb), C(62e33ba258712d51), C(fa085c15d779c0e), C(2c15d9142308c5ad), C(feb517011f27be9e),
    C(62e33ba258712d51), C(fa085c15d779c0e), C(2c15d9142308c5ad), C(feb517011f27be9e),
    C(1b2b049793b9eedb), C(d26be505fabc5a8f), C(adc483e42a5c36c5), C(c81ff37d56d3b00b)
  },
  {
    C(5029700a7773c3a4), C(d01231e97e300d0f), C(397cdc80f1f0ec58), C(e4041579de57c879), C(bbf513cb7bab5553), C(66ad0373099d5fa0), C(44bb6b21b87f3407),
    C(e4041579de57c879), C(bbf513cb7bab5553), C(66ad0373099d5fa0), C(44bb6b21b87f3407),
    C(a8108c43b4daba33), C(c0b5308c311e865e), C(cdd265ada48f6fcf), C(efbc1dae0a95ac0a)
  },
  {
    C(71c8287225d96c9a), C(eb836740524735c4), C(4777522d0e09846b), C(16fde90d02a1343b), C(ad14e0ed6e165185), C(8df6e0b2f24085dd), C(caa8a47292d50263),
    C(16fde90d02a1343b), C(ad14e0ed6e165185), C(8df6e0b2f24085dd), C(caa8a47292d50263),
    C(a020413ba660359d), C(9de401413f7c8a0c), C(20bfb965927a7c85), C(b52573e5f817ae27)
  },
  {
    C(4e8b9ad9347d7277), C(c0f195eeee7641cf), C(dbd810bee1ad5e50), C(8459801016414808), C(6fbf75735353c2d1), C(6e69aaf2d93ed647), C(85bb5b90167cce5e),
    C(8459801016414808), C(6fbf75735353c2d1), C(6e69aaf2d93ed647), C(85bb5b90167cce5e),
    C(39d79ee490d890cc), C(ac9f31f7ec97deb0), C(3bdc1cae4ed46504), C(eb5c63cfaee05622)
  },
  {
    C(1d5218d6ee2e52ab), C(cb25025c4daeff3b), C(aaf107566f31bf8c), C(aad20d70e231582b), C(eab92d70d9a22e54), C(cc5ab266375580c0), C(85091463e3630dce),
    C(aad20d70e231582b), C(eab92d70d9a22e54), C(cc5ab266375580c0), C(85091463e3630dce),
    C(b830b617a4690089), C(9dacf13cd76f13cf), C(d47cc5224265c68f), C(f04690880202b002)
  },
  {
    C(162360be6c293c8b), C(ff672b4a831953c8), C(dda57487ab6f78b5), C(38a42e0db55a4275), C(585971da56bb56d6), C(cd957009adc1482e), C(d6a96021e427567d),
    C(38a42e0db55a4275), C(585971da56bb56d6), C(cd957009adc1482e), C(d6a96021e427567d),
    C(8e2b1a5a63cd96fe), C(426ef8ce033d722d), C(c4d1c3d8acdda5f), C(4e694c9be38769b2)
  },
  {
    C(31459914f13c8867), C(ef96f4342d3bef53), C(a4e944ee7a1762fc), C(3526d9b950a1d910), C(a58ba01135bca7c0), C(cbad32e86d60a87c), C(adde1962aad3d730),
    C(3526d9b950a1d910), C(a58ba01135bca7c0), C(cbad32e86d60a87c), C(adde1962aad3d730),
    C(55faade148929704), C(bfc06376c72a2968), C(97762698b87f84be), C(117483d4828cbaf7)
  },
  {
    C(6b4e8fca9b3aecff), C(3ea0a33def0a296c), C(901fcb5fe05516f5), C(7c909e8cd5261727), C(c5acb3d5fbdc832e), C(54eff5c782ad3cdd), C(9d54397f3caf5bfa),
    C(7c909e8cd5261727), C(c5acb3d5fbdc832e), C(54eff5c782ad3cdd), C(9d54397f3caf5bfa),
    C(6b53ce24c4fc3092), C(2789abfdd4c9a14d), C(94d6a2261637276c), C(648aa4a2a1781f25)
  },
  {
    C(dd3271a46c7aec5d), C(fb1dcb0683d711c3), C(240332e9ebe5da44), C(479f936b6d496dca), C(dc2dc93d63739d4a), C(27e4151c3870498c), C(3a3a22ba512d13ba),
    C(479f936b6d496dca), C(dc2dc93d63739d4a), C(27e4151c3870498c), C(3a3a22ba512d13ba),
    C(5da92832f96d3cde), C(439b9ad48c4e7644), C(d2939279030accd9), C(6829f920e2950dbe)
  },
  {
    C(109b226238347d6e), C(e27214c32c43b7e7), C(eb71b0afaf0163ef), C(464f1adf4c68577), C(acf3961e1c9d897f), C(985b01ab89b41fe1), C(6972d6237390aac0),
    C(464f1adf4c68577), C(acf3961e1c9d897f), C(985b01ab89b41fe1), C(6972d6237390aac0),
    C(122d89898e256a0e), C(ac830561bd8be599), C(5744312574fbf0ad), C(7bff7f480a924ce9)
  },
  {
    C(cc920608aa94cce4), C(d67efe9e097bce4f), C(5687727c2c9036a9), C(8af42343888843c), C(191433ffcbab7800), C(7eb45fc94f88a71), C(31bc5418ffb88fa8),
    C(8af42343888843c), C(191433ffcbab7800), C(7eb45fc94f88a71), C(31bc5418ffb88fa8),
    C(4b53a37d8f446cb7), C(a6a7dfc757a60d28), C(a074be7bacbc013a), C(cc6db5f270de7adc)
  },
  {
    C(901ff46f22283dbe), C(9dd59794d049a066), C(3c7d9c3b0e77d2c6), C(dc46069eec17bfdf), C(cacb63fe65d9e3e), C(362fb57287d530c6), C(5854a4fbe1762d9),
    C(dc46069eec17bfdf), C(cacb63fe65d9e3e), C(362fb57287d530c6), C(5854a4fbe1762d9),
    C(3197427495021efc), C(5fabf34386aa4205), C(ca662891de36212), C(21f603e4d39bca84)
  },
  {
    C(11b3bdda68b0725d), C(2366bf0aa97a00bd), C(55dc4a4f6bf47e2b), C(69437142dae5a255), C(f2980cc4816965ac), C(dbbe76ba1d9adfcf), C(49c18025c0a8b0b5),
    C(69437142dae5a255), C(f2980cc4816965ac), C(dbbe76ba1d9adfcf), C(49c18025c0a8b0b5),
    C(fe25c147c9001731), C(38b99cad0ca30c81), C(c7ff06ac47eb950), C(a10f92885a6b3c02)
  },
  {
    C(9f5f03e84a40d232), C(1151a9ff99da844), C(d6f2e7c559ac4657), C(5e351e20f30377bf), C(91b3805daf12972c), C(94417fa6452a265e), C(bfa301a26765a7c),
    C(5e351e20f30377bf), C(91b3805daf12972c), C(94417fa6452a265e), C(bfa301a26765a7c),
    C(6924e2a053297d13), C(ed4a7904ed30d77e), C(d734abaad66d6eab), C(ce373e6c09e6e8a1)
  },
  {
    C(39eeff4f60f439be), C(1f7559c118517c70), C(6139d2492237a36b), C(fd39b7642cecf78f), C(104f1af4e9201df5), C(ab1a3cc7eaeab609), C(cee3363f210a3d8b),
    C(fd39b7642cecf78f), C(104f1af4e9201df5), C(ab1a3cc7eaeab609), C(cee3363f210a3d8b),
    C(51490f65fe56c884), C(6a8c8322cda993c), C(1f90609a017de1f0), C(9f3acea480a41edf)
  },
  {
    C(9b9e0126fe4b8b04), C(6a6190d520886c41), C(69640b27c16b3ed8), C(18865ff87619fd8f), C(dec5293e665663d8), C(ea07c345872d3201), C(6fce64da038a17ab),
    C(18865ff87619fd8f), C(dec5293e665663d8), C(ea07c345872d3201), C(6fce64da038a17ab),
    C(ad48f3c826c6a83e), C(70a1ff080a4da737), C(ecdac686c7d7719), C(700338424b657470)
  },
  {
    C(3ec4b8462b36df47), C(ff8de4a1cbdb7e37), C(4ede0449884716ac), C(b5f630ac75a8ce03), C(7cf71ae74fa8566a), C(e068f2b4618df5d), C(369df952ad3fd0b8),
    C(b5f630ac75a8ce03), C(7cf71ae74fa8566a), C(e068f2b4618df5d), C(369df952ad3fd0b8),
    C(5e1ba38fea018eb6), C(5ea5edce48e3da30), C(9b3490c941069dcb), C(e17854a44cc2fff)
  },
  {
    C(5e3fd9298fe7009f), C(d2058a44222d5a1d), C(cc25df39bfeb005c), C(1b0118c5c60a99c7), C(6ae919ef932301b8), C(cde25defa089c2fc), C(c2a3776e3a7716c4),
    C(1b0118c5c60a99c7), C(6ae919ef932301b8), C(cde25defa089c2fc), C(c2a3776e3a7716c4),
    C(2557bf65fb26269e), C(b2edabba58f2ae4f), C(264144e9f0e632cb), C(ad6481273c979566)
  },
  {
    C(7504ecb4727b274e), C(f698cfed6bc11829), C(71b62c425ecd348e), C(2a5e555fd35627db), C(55d5da439c42f3b8), C(a758e451732a1c6f), C(18caa6b46664b484),
    C(2a5e555fd35627db), C(55d5da439c42f3b8), C(a758e451732a1c6f), C(18caa6b46664b484),
    C(6ec1c7d1524bbad7), C(1cc3531dc422529d), C(61a6eeb29c0e5110), C(9cc8652016784a6a)
  },
  {
    C(4bdedc104d5eaed5), C(531c4bb4fd721e5d), C(1d860834e94a219f), C(1944ec723253392b), C(7ea6aa6a2f278ea5), C(5ff786af8113b3d5), C(194832eb9b0b8d0f),
    C(1944ec723253392b), C(7ea6aa6a2f278ea5), C(5ff786af8113b3d5), C(194832eb9b0b8d0f),
    C(56ab0396ed73fd38), C(2c88725b3dfbf89d), C(7ff57adf6275c816), C(b32f7630bcdb218)
  },
  {
    C(da0b4a6fb26a4748), C(8a3165320ae1af74), C(4803664ee3d61d09), C(81d90ddff0d00fdb), C(2c8c7ce1173b5c77), C(18c6b6c8d3f91dfb), C(415d5cbbf7d9f717),
    C(81d90ddff0d00fdb), C(2c8c7ce1173b5c77), C(18c6b6c8d3f91dfb), C(415d5cbbf7d9f717),
    C(b683e956f1eb3235), C(43166dde2b64d11f), C(f9689c90f5aad771), C(ca0ebc253c2eec38)
  },
  {
    C(bad6dd64d1b18672), C(6d4c4b91c68bd23f), C(d8f1507176822db7), C(381068e0f65f708b), C(b4f3762e451b12a6), C(6d61ed2f6d4e741), C(8b3b9df537b91a2c),
    C(381068e0f65f708b), C(b4f3762e451b12a6), C(6d61ed2f6d4e741), C(8b3b9df537b91a2c),
    C(b0759e599a91575c), C(9e7adbcc77212239), C(cf0eba98436555fe), C(b1fcc9c42c4cd1e6)
  },
  {
    C(98da3fe388d5860e), C(14a9fda8b3adb103), C(d85f5f798637994b), C(6e8e8ff107799274), C(24a2ef180891b531), C(c0eaf33a074bcb9d), C(1fa399a82974e17e),
    C(6e8e8ff107799274), C(24a2ef180891b531), C(c0eaf33a074bcb9d), C(1fa399a82974e17e),
    C(e7c116bef933725d), C(859908c7d17b93de), C(f6cfa27113af4a72), C(edf41c5d83c721a8)
  },
  {
    C(ef243a576431d7ac), C(92a32619ecfae0a5), C(fb34d2c062dc803a), C(f5f8b21ec30bd3a0), C(80a442fd5c6482a8), C(4fde11e5ccde5169), C(55671451f661a885),
    C(f5f8b21ec30bd3a0), C(80a442fd5c6482a8), C(4fde11e5ccde5169), C(55671451f661a885),
    C(94f27bc2d5d8d63e), C(2156968b87f084dc), C(b591bcae146f6fea), C(f57f4c01e41ac7fe)
  },
  {
    C(97854de6f22c97b6), C(1292ac07b0f426bb), C(9a099a28b22d3a38), C(caac64f5865d87f3), C(771b9fdbd3aa4bd2), C(88446393c3606c2d), C(bc3d3dcd5b7d6d7f),
    C(caac64f5865d87f3), C(771b9fdbd3aa4bd2), C(88446393c3606c2d), C(bc3d3dcd5b7d6d7f),
    C(56e22512b832d3ee), C(bbc677fe5ce0b665), C(f1914b0f070e5c32), C(c10d40362472dcd1)
  },
  {
    C(d26ce17bfc1851d), C(db30fb632c7da294), C(26cb7b1a465400a5), C(401a0581221957e2), C(fc04e99ae3a283ce), C(fe895303ab2d1e3e), C(35ab7c498403975b),
    C(401a0581221957e2), C(fc04e99ae3a283ce), C(fe895303ab2d1e3e), C(35ab7c498403975b),
    C(c6e4c8dc6f52fb11), C(63f0b484c2c7502f), C(93693da3439bdbe9), C(1264dbaaaaf6b7f1)
  },
  {
    C(97477bac0ba4c7f1), C(788ef8729dca29ac), C(63d88e226d36132c), C(330b7e93663affbd), C(3c59913fcf0d603f), C(e207e6572672fd0a), C(8a5dc17019c8a667),
    C(330b7e93663affbd), C(3c59913fcf0d603f), C(e207e6572672fd0a), C(8a5dc17019c8a667),
    C(5c8f47ade659d40), C(6e0838e5a808e9a2), C(8a2d9a0afcd48b19), C(d1c9d5af7b48418d)
  },
  {
    C(f6bbcba92b11f5c8), C(72cf221cad20f191), C(a04726593764122d), C(77fbb70409d316e2), C(c864432c5208e583), C(d3f593922668c184), C(23307562648bdb54),
    C(77fbb70409d316e2), C(c864432c5208e583), C(d3f593922668c184), C(23307562648bdb54),
    C(b03e0b274f848a74), C(c6121e3af71f4281), C(2e48dd2a16ca63ec), C(f4cd44c69ae024df)
  },
  {
    C(1ac8b67c1c82132), C(7536db9591be9471), C(42f18fbe7141e565), C(20085827a39ff749), C(42e6c504df174606), C(839da16331fea7ac), C(7fd768552b10ffc6),
    C(20085827a39ff749), C(42e6c504df174606), C(839da16331fea7ac), C(7fd768552b10ffc6),
    C(d1c53c90fde72640), C(c61ae7cf4e266556), C(127561e440e4c156), C(f329cae8c26af3e1)
  },
  {
    C(9cd716ca0eee52fa), C(67c1076e1ef11f93), C(927342024f36f5d7), C(d0884af223fd056b), C(bb33aafc7b80b3e4), C(36b722fea81a4c88), C(6e72e3022c0ed97),
    C(d0884af223fd056b), C(bb33aafc7b80b3e4), C(36b722fea81a4c88), C(6e72e3022c0ed97),
    C(5db446a3ba66e0ba), C(2e138fb81b28ad9), C(16e8e82995237c85), C(9730dbfb072fbf03)
  },
  {
    C(1909f39123d9ad44), C(c0bdd71c5641fdb7), C(112e5d19abda9b14), C(984cf3f611546e28), C(d7d9c9c4e7efb5d7), C(b3152c389532b329), C(1c168b512ec5f659),
    C(984cf3f611546e28), C(d7d9c9c4e7efb5d7), C(b3152c389532b329), C(1c168b512ec5f659),
    C(eca67cc49e26069a), C(73cb0b224d36d541), C(df8379190ae6c5fe), C(e0f6bde7c4726211)
  },
  {
    C(1d206f99f535efeb), C(882e15548afc3422), C(c94f203775c8c634), C(24940a3adac420b8), C(5adf73051c52bce0), C(1aa5030247ed3d32), C(e1ae74ab6804c08b),
    C(24940a3adac420b8), C(5adf73051c52bce0), C(1aa5030247ed3d32), C(e1ae74ab6804c08b),
    C(95217bf71b0da84c), C(ca9bb91c0126a36e), C(741b9a99ea470974), C(2adc4e34b8670f41)
  },
  {
    C(b38c3a83042eb802), C(ea134be7c6e0c326), C(81d396c683df4f35), C(2a55645640911e27), C(4fac2eefbd36e26f), C(79ad798fb4c5835c), C(359aa2faec050131),
    C(2a55645640911e27), C(4fac2eefbd36e26f), C(79ad798fb4c5835c), C(359aa2faec050131),
    C(5b802dcec21a7157), C(6ecde915b75ede0a), C(f2e653587e89058b), C(a661be80528d3385)
  },
  {
    C(488d6b45d927161b), C(f5cac66d869a8aaf), C(c326d56c643a214e), C(10a7228693eb083e), C(1054fb19cbacf01c), C(a8f389d24587ebd8), C(afcb783a39926dba),
    C(10a7228693eb083e), C(1054fb19cbacf01c), C(a8f389d24587ebd8), C(afcb783a39926dba),
    C(fe83e658532edf8f), C(6fdcf97f147dc4db), C(dc5e487845abef4b), C(137693f4eab77e27)
  },
  {
    C(3d6aaa43af5d4f86), C(44c7d370910418d8), C(d099515f7c5c4eca), C(39756960441fbe2f), C(fb68e5fedbe3d874), C(3ff380fbdd27b8e), C(f48832fdda648998),
    C(39756960441fbe2f), C(fb68e5fedbe3d874), C(3ff380fbdd27b8e), C(f48832fdda648998),
    C(270ddbf2327058c9), C(9eead83a8319d0c4), C(b4c3356e162b086d), C(88f013588f411b7)
  },
  {
    C(e5c40a6381e43845), C(312a18e66bbceaa3), C(31365186c2059563), C(cba4c10e65410ba0), C(3c250c8b2d72c1b6), C(177e82f415595117), C(8c8dcfb9e73d3f6),
    C(cba4c10e65410ba0), C(3c250c8b2d72c1b6), C(177e82f415595117), C(8c8dcfb9e73d3f6),
    C(c017a797e49c0f7), C(ea2b233b2e7d5aea), C(878d204c55a56cb1), C(7b1b62cc0dfdc523)
  },
  {
    C(86fb323e5a4b710b), C(710c1092c23a79e0), C(bd2c6d3fc949402e), C(951f2078aa4b8099), C(e68b7fefa1cfd190), C(41525a4990ba6d4a), C(c373552ef4b51712),
    C(951f2078aa4b8099), C(e68b7fefa1cfd190), C(41525a4990ba6d4a), C(c373552ef4b51712),
    C(73eb44c6122bdf5a), C(58047289a314b013), C(e31d30432521705b), C(6cf856774873faa4)
  },
  {
    C(7930c09adaf6e62e), C(f230d3311593662c), C(a795b9bf6c37d211), C(b57ec44bc7101b96), C(6cb710e77767a25a), C(2f446152d5e3a6d0), C(cd69172f94543ce3),
    C(b57ec44bc7101b96), C(6cb710e77767a25a), C(2f446152d5e3a6d0), C(cd69172f94543ce3),
    C(e6c2483cf425f072), C(2060d5d4379d6d5a), C(86a3c04c2110d893), C(561d3b8a509313c6)
  },
  {
    C(e505e86f0eff4ecd), C(cf31e1ccb273b9e6), C(d8efb8e9d0fe575), C(ed094f47671e359d), C(d9ebdb047d57611a), C(1c620e4d301037a3), C(df6f401c172f68e8),
    C(ed094f47671e359d), C(d9ebdb047d57611a), C(1c620e4d301037a3), C(df6f401c172f68e8),
    C(af0a2c7f72388ec7), C(6d4c4a087fa4564a), C(411b30def69700a), C(67e5c84557a47e01)
  },
  {
    C(dedccb12011e857), C(d831f899174feda8), C(ee4bcdb5804c582a), C(5d765af4e88f3277), C(d2abe1c63ad4d103), C(342a8ce0bc7af6e4), C(31bfda956f3e5058),
    C(5d765af4e88f3277), C(d2abe1c63ad4d103), C(342a8ce0bc7af6e4), C(31bfda956f3e5058),
    C(4c7a1fec9af54bbb), C(84a88f0655899bf4), C(66fb60d0582ac601), C(be0dd1ffe967bd4a)
  },
  {
    C(4d679bda26f5555f), C(7deb387eb7823c1c), C(a65ef3b4fecd6888), C(a6814d3dc578b9df), C(3372111a3292b691), C(e97589c81d92b513), C(74edd943d1b9b5bf),
    C(a6814d3dc578b9df), C(3372111a3292b691), C(e97589c81d92b513), C(74edd943d1b9b5bf),
    C(889e38b0af80bb7a), C(a416349af3c5818b), C(f5f5bb25576221c1), C(3be023fa6912c32e)
  },
  {
    C(e47cd22995a75a51), C(3686350c2569a162), C(861afcb185b8efd9), C(63672de7951e1853), C(3ca0c763273b99db), C(29e04fa994cccb98), C(b02587d792be5ee8),
    C(63672de7951e1853), C(3ca0c763273b99db), C(29e04fa994cccb98), C(b02587d792be5ee8),
    C(c85ada4858f7e4fc), C(3f280ab7d5864460), C(4109822f92f68326), C(2d73f61314a2f630)
  },
  {
    C(92ba8e12e0204f05), C(4e29321580273802), C(aa83b675ed74a851), C(a16cd2e8b445a3fd), C(f0d4f9fb613c38ef), C(eee7755d444d8f2f), C(b530591eb67ae30d),
    C(a16cd2e8b445a3fd), C(f0d4f9fb613c38ef), C(eee7755d444d8f2f), C(b530591eb67ae30d),
    C(6fb3031a6edf8fec), C(65118d08aecf56d8), C(9a2117bbef1faa8), C(97055c5fd310aa93)
  },
  {
    C(bb3a8427c64f8939), C(b5902af2ec095a04), C(89f1b440667b2a28), C(5386ef0b438d0330), C(d39e03c686f8a2da), C(9555249bb9073d78), C(8c0b3623fdf0b156),
    C(5386ef0b438d0330), C(d39e03c686f8a2da), C(9555249bb9073d78), C(8c0b3623fdf0b156),
    C(354fc5d3a5504e5e), C(b2fd7391719aa614), C(13cd4ce3dfe27b3d), C(a2d63a85dc3cae4b)
  },
  {
    C(998988f7d6dacc43), C(5f2b853d841152db), C(d76321badc5cb978), C(e381f24ee1d9a97d), C(7c5d95b2a3af2e08), C(ca714acc461cdc93), C(1a8ee94bc847aa3e),
    C(e381f24ee1d9a97d), C(7c5d95b2a3af2e08), C(ca714acc461cdc93), C(1a8ee94bc847aa3e),
    C(ee59ee4c21a36f47), C(d476e8bba5bf5143), C(22a03cb5900f6ec8), C(19d954e14f35d7a8)
  },
  {
    C(3f1049221dd72b98), C(8d9200d7a0664c37), C(3925704c83a5f406), C(4cbef49086e62678), C(d77dfecc2819ef19), C(c327e4deaf4c7e72), C(b4d58c73a262a32d),
    C(4cbef49086e62678), C(d77dfecc2819ef19), C(c327e4deaf4c7e72), C(b4d58c73a262a32d),
    C(78cd002324861653), C(7c3f3977576efb88), C(d1c9975fd4a4cc26), C(3e3cbc90a9baa442)
  },
  {
    C(419e4ff78c3e06f3), C(aa8ff514c8a141d7), C(5bb176e21f89f10d), C(becb065dc12d8b4e), C(ebee135492a2018), C(d3f07e65bcd9e13a), C(85c933e85382e9f9),
    C(becb065dc12d8b4e), C(ebee135492a2018), C(d3f07e65bcd9e13a), C(85c933e85382e9f9),
    C(2c19ab7c419ebaca), C(982375b2999bdb46), C(652ca1c6325d9296), C(e9c790fa8561940a)
  },
  {
    C(9ba090af14171317), C(b0445c5232d7be53), C(72cc929d1577ddb8), C(bc944c1b5ba2184d), C(ab3d57e5e60e9714), C(5d8d27e7dd0a365a), C(4dd809e11740af1a),
    C(bc944c1b5ba2184d), C(ab3d57e5e60e9714), C(5d8d27e7dd0a365a), C(4dd809e11740af1a),
    C(6f42d856faad44df), C(5118dc58d7eaf56e), C(829bbc076a43004), C(1747fbbfaca6da98)
  },
  {
    C(6ad739e4ada9a340), C(2c6c4fb3a2e9b614), C(ab58620e94ca8a77), C(aaa144fbe3e6fda2), C(52a9291d1e212bc5), C(2b4c68291f26b570), C(45351ab332855267),
    C(aaa144fbe3e6fda2), C(52a9291d1e212bc5), C(2b4c68291f26b570), C(45351ab332855267),
    C(1149f55400bc9799), C(8c6ec1a0c617771f), C(e9966cc03f3bec05), C(3e6889140ccd2646)
  },
  {
    C(8ecff07fd67e4abd), C(f1b8029b17006ece), C(21d96d5859229a61), C(b8c18d66154ac51), C(5807350371ad7388), C(81f783f4f5ab2b8), C(fa4e659f90744de7),
    C(b8c18d66154ac51), C(5807350371ad7388), C(81f783f4f5ab2b8), C(fa4e659f90744de7),
    C(809da4baa51cad2c), C(88d5c11ff5598342), C(7c7125b0681d67d0), C(1b5ba6124bfed8e8)
  },
  {
    C(497ca8dbfee8b3a7), C(58c708155d70e20e), C(90428a7e349d6949), C(b744f5056e74ca86), C(88aa27b96f3d84a5), C(b4b1ee0470ac3826), C(aeb46264f4e15d4f),
    C(b744f5056e74ca86), C(88aa27b96f3d84a5), C(b4b1ee0470ac3826), C(aeb46264f4e15d4f),
    C(14921b1ee856bc55), C(a341d74aaba00a02), C(4f50aa8e3d08a919), C(75a148668ff3869e)
  },
  {
    C(a929cd66daa65b0a), C(7c0150a2d9ca564d), C(46ddec37e2ec0a6d), C(4323852cc57e4af3), C(1f5f638bbf9d2e5b), C(578fb6ac89a31d9), C(7792536d9ac4bf12),
    C(4323852cc57e4af3), C(1f5f638bbf9d2e5b), C(578fb6ac89a31d9), C(7792536d9ac4bf12),
    C(60be62e795ef5798), C(c276cc5b44febefe), C(519ba0b9f6d1be95), C(1fdce3561ed35bb8)
  },
  {
    C(4107c4156bc8d4bc), C(1cda0c6f3f0f48af), C(cf11a23299cf7181), C(766b71bff7d6f461), C(b004f2c910a6659e), C(4c0eb3848e1a7c8), C(3f90439d05c3563b),
    C(766b71bff7d6f461), C(b004f2c910a6659e), C(4c0eb3848e1a7c8), C(3f90439d05c3563b),
    C(4a2a013f4bc2c1d7), C(888779ab0c272548), C(ae0f8462d89a4241), C(c5c85b7c44679abd)
  },
  {
    C(15b38dc0e40459d1), C(344fedcfc00fff43), C(b9215c5a0fcf17df), C(d178444a236c1f2d), C(5576deee27f3f103), C(943611bb5b1b0736), C(a0fde17cb5c2316d),
    C(d178444a236c1f2d), C(5576deee27f3f103), C(943611bb5b1b0736), C(a0fde17cb5c2316d),
    C(feaa1a047f4375f3), C(5435f313e84767e), C(522e4333cd0330c1), C(7e6b609b0ea9e91f)
  },
  {
    C(e5e5370ed3186f6c), C(4592e75db47ea35d), C(355d452b82250e83), C(7a265e37da616168), C(6a1f06c34bafa27), C(fbae175e7ed22a9c), C(b144e84f6f33c098),
    C(7a265e37da616168), C(6a1f06c34bafa27), C(fbae175e7ed22a9c), C(b144e84f6f33c098),
    C(bd444561b0db41fc), C(2072c85731e7b0b0), C(ce1b1fac436b51f3), C(4f5d44f31a3dcdb9)
  },
  {
    C(ea2785c8f873e28f), C(3e257272f4464f5f), C(9267e7e0cc9c7fb5), C(9fd4d9362494cbbc), C(e562bc615befb1b9), C(8096808d8646cfde), C(c4084a587b9776ec),
    C(9fd4d9362494cbbc), C(e562bc615befb1b9), C(8096808d8646cfde), C(c4084a587b9776ec),
    C(a9135db8a850d8e4), C(fffc4f8b1a11f5af), C(c50e9173c2c6fe64), C(a32630581df4ceda)
  },
  {
    C(e7bf98235fc8a4a8), C(4042ef2aae400e64), C(6538ba9ffe72dd70), C(c84bb7b3881ab070), C(36fe6c51023fbda0), C(d62838514bb87ea4), C(9eeb5e7934373d86),
    C(c84bb7b3881ab070), C(36fe6c51023fbda0), C(d62838514bb87ea4), C(9eeb5e7934373d86),
    C(5f8480d0a2750a96), C(40afa38506456ad9), C(e4012b7ef2e0ddea), C(659da200a011836b)
  },
  {
    C(b94e261a90888396), C(1f468d07e853294c), C(cb2c9b863a5317b9), C(4473c8e2a3458ee0), C(258053945ab4a39a), C(f8d745ca41962817), C(7afb6d40df9b8f71),
    C(4473c8e2a3458ee0), C(258053945ab4a39a), C(f8d745ca41962817), C(7afb6d40df9b8f71),
    C(9030c2349604f677), C(f544dcd593087faf), C(77a3b0efe6345d12), C(fff4e398c05817cc)
  },
  {
    C(4b0226e5f5cdc9c), C(a836ae7303dc4301), C(8505e1b628bac101), C(b5f52041a698da7), C(29864874b5f1936d), C(49b3a0c6d78f98da), C(93a1a8c7d90de296),
    C(b5f52041a698da7), C(29864874b5f1936d), C(49b3a0c6d78f98da), C(93a1a8c7d90de296),
    C(ed62288423c17b7f), C(685afa2cfba09660), C(6d9b6f59585452c6), C(e505535c4010efb9)
  },
  {
    C(e07edbe7325c718c), C(9db1eda964f06827), C(2f245ad774e4cb1b), C(664ec3fad8521859), C(406f082beb9ca29a), C(b6b0fb3a7981c7c8), C(3ebd280b598a9721),
    C(664ec3fad8521859), C(406f082beb9ca29a), C(b6b0fb3a7981c7c8), C(3ebd280b598a9721),
    C(d9a6ceb072eab22a), C(d5bc5df5eb2ff6f1), C(488db3cab48daa0b), C(9916f14fa5672f37)
  },
  {
    C(f4b56421eae4c4e7), C(5da0070cf40937a0), C(aca4a5e01295984a), C(5414e385f5677a6d), C(41ef105f8a682a28), C(4cd2e95ea7f5e7b0), C(775bb1e0d57053b2),
    C(5414e385f5677a6d), C(41ef105f8a682a28), C(4cd2e95ea7f5e7b0), C(775bb1e0d57053b2),
    C(8919017805e84b3f), C(15402f44e0e2b259), C(483b1309e1403c87), C(85c7b4232d45b0d9)
  },
  {
    C(c07fcb8ae7b4e480), C(4ebcad82e0b53976), C(8643c63d6c78a6ce), C(d4bd358fed3e6aa5), C(8a1ba396356197d9), C(7afc2a54733922cc), C(b813bdac4c7c02ef),
    C(d4bd358fed3e6aa5), C(8a1ba396356197d9), C(7afc2a54733922cc), C(b813bdac4c7c02ef),
    C(f6c610cf7e7c955), C(dab6a53e1c0780f8), C(837c9ffec33e5d48), C(8cb8c20032af152d)
  },
  {
    C(3edad9568a9aaab), C(23891bbaeb3a17bc), C(4eb7238738b0c51a), C(db0c32f76f5b7fc1), C(5e41b711f0abd1a0), C(bcb758f01ded0a11), C(7d15f7d87955e28b),
    C(db0c32f76f5b7fc1), C(5e41b711f0abd1a0), C(bcb758f01ded0a11), C(7d15f7d87955e28b),
    C(cd2dc1f0b05939b), C(9fd6d680462e4c47), C(95d5846e993bc8ff), C(f0b3cafc2697b8a8)
  },
  {
    C(fcabde8700de91e8), C(63784d19c60bf366), C(8f3af9a056b1a1c8), C(32d3a29cf49e2dc9), C(3079c0b0c2269bd0), C(ed76ba44f04e7b82), C(6eee76a90b83035f),
    C(32d3a29cf49e2dc9), C(3079c0b0c2269bd0), C(ed76ba44f04e7b82), C(6eee76a90b83035f),
    C(4a9286f545bbc09), C(bd36525be4dd1b51), C(5f7a9117228fdee5), C(543c96a08f03151c)
  },
  {
    C(362fc5ba93e8eb31), C(7549ae99fa609d61), C(47e4cf524e37178f), C(a54eaa5d7f3a7227), C(9d26922965d54727), C(27d22acb31a194d4), C(e9b8e68771db0da6),
    C(a54eaa5d7f3a7227), C(9d26922965d54727), C(27d22acb31a194d4), C(e9b8e68771db0da6),
    C(16fd0e006209abe8), C(81d3f72987a6a81a), C(74e96e4044817bc7), C(924ca5f08572fef9)
  },
  {
    C(e323b1c5b55a4dfb), C(719993d7d1ad77fb), C(555ca6c6166e989c), C(ea37f61c0c2f6d53), C(9b0c2174f14a01f5), C(7bbe6921e26293f3), C(2ab6c72235b6c98a),
    C(ea37f61c0c2f6d53), C(9b0c2174f14a01f5), C(7bbe6921e26293f3), C(2ab6c72235b6c98a),
    C(2c6e7668f37f6d23), C(3e8edb057a57c2dd), C(2595fc79768c8b34), C(ffc541f5efed9c43)
  },
  {
    C(9461913a153530ef), C(83fc6d9ed7d1285a), C(73df90bdc50807cf), C(a32c192f6e3c3f66), C(8f10077b8a902d00), C(61a227f2faac29b4), C(1a71466fc005a61d),
    C(a32c192f6e3c3f66), C(8f10077b8a902d00), C(61a227f2faac29b4), C(1a71466fc005a61d),
    C(12545812f3d01a92), C(aece72f823ade07d), C(52634cdd5f9e5260), C(cb48f56805c08e98)
  },
  {
    C(ec2332acc6df0c41), C(59f5ee17e20a8263), C(1087d756afcd8e7b), C(a82a7bb790678fc9), C(d197682c421e4373), C(dd78d25c7f0f935a), C(9850cb6fbfee520f),
    C(a82a7bb790678fc9), C(d197682c421e4373), C(dd78d25c7f0f935a), C(9850cb6fbfee520f),
    C(2590847398688a46), C(ad266f08713ca5fe), C(25b978be91e830b5), C(2996c8f2b4c8f231)
  },
  {
    C(aae00b3a289bc82), C(4f6d69f5a5a5b659), C(3ff5abc145614e3), C(33322363b5f45216), C(7e83f1fe4189e843), C(df384b2adfc35b03), C(396ce7790a5ada53),
    C(33322363b5f45216), C(7e83f1fe4189e843), C(df384b2adfc35b03), C(396ce7790a5ada53),
    C(c3286e44108b8d36), C(6db8716c498d703f), C(d1db09466f37f4e7), C(56c98e7f68a41388)
  },
  {
    C(4c842e732fcd25f), C(e7dd7b953cf9c2b2), C(911ee248a76ae3), C(33c6690937582317), C(fe6d61a77985d7bb), C(97b153d04a115535), C(d3fde02e42cfe6df),
    C(33c6690937582317), C(fe6d61a77985d7bb), C(97b153d04a115535), C(d3fde02e42cfe6df),
    C(d1c7d1efa52a016), C(1d6ed137f4634c), C(1a260ec505097081), C(8d1e70861a1c7db6)
  },
  {
    C(40e23ca5817a91f3), C(353e2935809b7ad1), C(f7820021b86391bb), C(f3d41b3d4717eb83), C(2670d457dde68842), C(19707a6732c49278), C(5d0f05a83569ba26),
    C(f3d41b3d4717eb83), C(2670d457dde68842), C(19707a6732c49278), C(5d0f05a83569ba26),
    C(6fe5bc84e528816a), C(94df3dca91a29ace), C(420196ed097e8b6f), C(7c52da0e1f043ad6)
  },
  {
    C(2564527fad710b8d), C(2bdcca8d57f890f), C(81f7bfcd9ea5a532), C(dd70e407984cfa80), C(66996d6066db6e1a), C(36a812bc418b97c9), C(18ea2c63da57f36e),
    C(dd70e407984cfa80), C(66996d6066db6e1a), C(36a812bc418b97c9), C(18ea2c63da57f36e),
    C(937fd7ad09be1a8f), C(163b12cab35d5d15), C(3606c3e441335cce), C(949f6ea5bb241ae8)
  },
  {
    C(6bf70df9d15a2bf6), C(81cad17764b8e0dd), C(58b349a9ba22a7ef), C(9432536dd9f65229), C(192dc54522da3e3d), C(274c6019e0227ca9), C(160abc932a4e4f35),
    C(9432536dd9f65229), C(192dc54522da3e3d), C(274c6019e0227ca9), C(160abc932a4e4f35),
    C(1204f2fb5aa79dc6), C(2536edaf890f0760), C(6f2b561f44ff46b4), C(8c7b3e95baa8d984)
  },
  {
    C(45e6f446eb6bbcf5), C(98ab0ef06f1a7d84), C(85ae96bacca50de6), C(b9aa5bead3352801), C(8a6d9e02a19a4229), C(c352f5b6d5ee1d9d), C(ce562bdb0cfa84fb),
    C(b9aa5bead3352801), C(8a6d9e02a19a4229), C(c352f5b6d5ee1d9d), C(ce562bdb0cfa84fb),
    C(d47b768a85283981), C(1fe72557be57a11b), C(95d8afe4af087d51), C(2f59c4e383f30045)
  },
  {
    C(620d3fe4b8849c9e), C(975a15812a429ec2), C(437c453593dcaf13), C(8d8e7c63385df78e), C(16d55add72a5e25e), C(aa6321421dd87eb5), C(6f27f62e785f0203),
    C(8d8e7c63385df78e), C(16d55add72a5e25e), C(aa6321421dd87eb5), C(6f27f62e785f0203),
    C(829030a61078206e), C(ae1f30fcfa445cc8), C(f61f21c9df4ef68d), C(1e5b1945f858dc4c)
  },
  {
    C(535aa7340b3c168f), C(bed5d3c3cd87d48a), C(266d40ae10f0cbc1), C(ce218d5b44f7825a), C(2ae0c64765800d3a), C(f22dc1ae0728fc01), C(48a171bc666d227f),
    C(ce218d5b44f7825a), C(2ae0c64765800d3a), C(f22dc1ae0728fc01), C(48a171bc666d227f),
    C(e7367aff24203c97), C(da39d2be1db3a58d), C(85ce86523003933a), C(dfd4ef2ae83f138a)
  },
  {
    C(dd3e761d4eada300), C(893d7e4c3bea5bb6), C(cc6d6783bf43eea), C(eb8eed7c391f0044), C(b58961c3abf80753), C(3d75ea687191521), C(389be7bbd8e478f3),
    C(eb8eed7c391f0044), C(b58961c3abf80753), C(3d75ea687191521), C(389be7bbd8e478f3),
    C(917070a07441ee47), C(d78efa8cd65b313), C(a8a16f4c1c08c8a1), C(b69cb8ee549eb113)
  },
  {
    C(4ac1902ccde06545), C(2c44aeb0983a7a07), C(b566035215b309f9), C(64c136fe9404a7b3), C(99f3d8c98a399d5e), C(6319c7cb14180185), C(fbacdbd277d33f4c),
    C(64c136fe9404a7b3), C(99f3d8c98a399d5e), C(6319c7cb14180185), C(fbacdbd277d33f4c),
    C(a96a5626c2adda86), C(39ea72fd2ad133ed), C(b5583f2f736df73e), C(ef2c63619782b7ba)
  },
  {
    C(aee339a23bb00a5e), C(cbb402255318f919), C(9922948e99aa0781), C(df367034233fedc4), C(dcbe14db816586e5), C(f4b1cb814adf21d3), C(f4690695102fa00a),
    C(df367034233fedc4), C(dcbe14db816586e5), C(f4b1cb814adf21d3), C(f4690695102fa00a),
    C(6b4f01dd6b76dafc), C(b79388676b50da5d), C(cb64f8bde5ed3393), C(9b422781f13219d3)
  },
  {
    C(627599e91148df4f), C(3e2d01e8baab062b), C(2daab20edb245251), C(9a958bc3a895a223), C(331058dd6c5d2064), C(46c4d962072094fa), C(e6207c19160e58eb),
    C(9a958bc3a895a223), C(331058dd6c5d2064), C(46c4d962072094fa), C(e6207c19160e58eb),
    C(5655e4dbf7272728), C(67b217b1f56c747d), C(3ac0be79691b9a0d), C(9d0954dd0b57073)
  },
  {
    C(cfb04cf00cfed6b3), C(5fe75fc559af22fa), C(c440a935d72cdc40), C(3ab0d0691b251b8b), C(47181a443504a819), C(9bcaf1253f99f499), C(8ee002b89c1b6b3f),
    C(3ab0d0691b251b8b), C(47181a443504a819), C(9bcaf1253f99f499), C(8ee002b89c1b6b3f),
    C(55dfe8eedcd1ec5e), C(1bf50f0bbad796a5), C(9044369a042d7fd6), C(d423df3e3738ba8f)
  },
  {
    C(942631c47a26889), C(427962c82d8a6e00), C(224071a6592537ff), C(d3e96f4fb479401), C(68b3f2ec11de9368), C(cb51b01083acad4f), C(500cec4564d62aeb),
    C(d3e96f4fb479401), C(68b3f2ec11de9368), C(cb51b01083acad4f), C(500cec4564d62aeb),
    C(4ce547491e732887), C(9423883a9a11df4c), C(1a0fc7a14214360), C(9e837914505da6ed)
  },
  {
    C(4c9eb4e09726b47e), C(fd927483a2b38cf3), C(6d7e56407d1ba870), C(9f5dc7db69fa1e29), C(f42fff56934533d5), C(92d768c230a53918), C(f3360ff11642136c),
    C(9f5dc7db69fa1e29), C(f42fff56934533d5), C(92d768c230a53918), C(f3360ff11642136c),
    C(9e989932eb86d1b5), C(449a77f69a8a9d65), C(efabaf8a7789ed9a), C(2798eb4c50c826fd)
  },
  {
    C(cf7f208ef20e887a), C(f4ce4edeadcaf1a1), C(7ee15226eaf4a74d), C(17ab41ab2ae0705d), C(9dd56694aa2dcd4e), C(dd4fa2add9baced2), C(7ad99099c9e199a3),
    C(17ab41ab2ae0705d), C(9dd56694aa2dcd4e), C(dd4fa2add9baced2), C(7ad99099c9e199a3),
    C(a59112144accef0e), C(5838df47e38d251d), C(8750fe45760331e5), C(4b2ce14732e0312a)
  },
  {
    C(a8dc4687bcf27f4), C(c4aadd7802553f15), C(5401eb9912be5269), C(5c2a2b5b0657a928), C(1e1968ebb38fcb99), C(a082d0e067c4a59c), C(18b616495ad9bf5d),
    C(5c2a2b5b0657a928), C(1e1968ebb38fcb99), C(a082d0e067c4a59c), C(18b616495ad9bf5d),
    C(18c5dc6c78a7f9ed), C(b3cc94fe34b68aa1), C(3b77e91292be38cc), C(61d1786ec5097971)
  },
  {
    C(daed638536ed19df), C(1a762ea5d7ac6f7e), C(48a1cc07a798b84f), C(7f15bdaf50d550f9), C(4c1d48aa621a037e), C(2b1d7a389d497ee0), C(81c6775d46f4b517),
    C(7f15bdaf50d550f9), C(4c1d48aa621a037e), C(2b1d7a389d497ee0), C(81c6775d46f4b517),
    C(35296005cbba3ebe), C(db1642f825b53532), C(3e07588a9fd829a4), C(60f13b5446bc7638)
  },
  {
    C(90a04b11ee1e4af3), C(ab09a35f8f2dff95), C(d7cbe82231ae1e83), C(3262e9017bb788c4), C(1612017731c997bc), C(e789d66134aff5e1), C(275642fd17048af1),
    C(3262e9017bb788c4), C(1612017731c997bc), C(e789d66134aff5e1), C(275642fd17048af1),
    C(99255b68d0b46b51), C(74a6f1ad4b2bb296), C(4164222761af840e), C(54d59bf6211a8fe6)
  },
  {
    C(511f29e1b732617d), C(551cb47a9a83d769), C(df6f56fbda20e7a), C(f27583a930221d44), C(d7d2c46de69b2ed8), C(add24ddd2be4a850), C(5cf2f688dbb93585),
    C(f27583a930221d44), C(d7d2c46de69b2ed8), C(add24ddd2be4a850), C(5cf2f688dbb93585),
    C(a7f8e42d5dd4aa00), C(72dc11fd76b4dea9), C(8886f194e6f8e3ff), C(7e8ead04a0e0b1ef)
  },
  {
    C(95567f03939e651f), C(62a426f09d81d884), C(15cb96e36a8e712c), C(1a2f43bdeaea9c28), C(bca2fd840831291f), C(83446d4a1f7dcc1a), C(449a211df83b6187),
    C(1a2f43bdeaea9c28), C(bca2fd840831291f), C(83446d4a1f7dcc1a), C(449a211df83b6187),
    C(553ce97832b2f695), C(3110a2ba303db75), C(b91d6d399a02f453), C(3cb148561e0ef2bb)
  },
  {
    C(248a32ad10e76bc3), C(dac39c8b036985e9), C(79d38c4af2958b56), C(cc954b4e56275f54), C(700cd864e04e8aaa), C(d6ba03cbff7cc34b), C(da297d7891c9c046),
    C(cc954b4e56275f54), C(700cd864e04e8aaa), C(d6ba03cbff7cc34b), C(da297d7891c9c046),
    C(c05d2be8f8ee8114), C(7f4541cbe2ec0025), C(8f0a7a70af6ea926), C(3837ddce693781b5)
  },
  {
    C(f9f05a2a892242eb), C(de00b6b2e0998460), C(f1f4bd817041497a), C(3deac49eb42a1e26), C(642f77f7c57e84b7), C(2f2c231222651e8b), C(380202ec06bdc29e),
    C(3deac49eb42a1e26), C(642f77f7c57e84b7), C(2f2c231222651e8b), C(380202ec06bdc29e),
    C(59abc4ff54765e66), C(8561ea1dddd1f742), C(9ca1f94b0d3f3875), C(b7fa93c3a9fa4ec4)
  },
  {
    C(3a015cea8c3f5bdf), C(5583521b852fc3ac), C(53d5cd66029a1014), C(ac2eeca7bb04412a), C(daba45cb16ccff2b), C(ddd90b51209e414), C(d90e74ee28cb6271),
    C(ac2eeca7bb04412a), C(daba45cb16ccff2b), C(ddd90b51209e414), C(d90e74ee28cb6271),
    C(117027648ca9db68), C(29c1dba39bbcf072), C(787f6bb010a34cd9), C(e099f487e09b847)
  },
  {
    C(670e43506aa1f71b), C(1cd7929573e54c05), C(cbb00a0aaba5f20a), C(f779909e3d5688d1), C(88211b9117678271), C(59f44f73759a8bc6), C(ef14f73c405123b4),
    C(f779909e3d5688d1), C(88211b9117678271), C(59f44f73759a8bc6), C(ef14f73c405123b4),
    C(78775601f11186f), C(fc4641d676fbeed9), C(669ca96b5a2ae5b), C(67b5f0d072025f8d)
  },
  {
    C(977bb79b58bbd984), C(26d45cfcfb0e9756), C(df8885db518d5f6a), C(6a1d2876488bed06), C(ae35d83c3afb5769), C(33667427d99f9f4e), C(d84c31c17495e3ba),
    C(6a1d2876488bed06), C(ae35d83c3afb5769), C(33667427d99f9f4e), C(d84c31c17495e3ba),
    C(31357cded7495ffc), C(295e2eefcd383a2e), C(25063ef4a24c29ae), C(88c694170fcbf0b7)
  },
  {
    C(e6264fbccd93a530), C(c92f420494e99a7d), C(c14001a298cf976), C(5c8685fee2e4ce55), C(228c49268d6a4345), C(3b04ee2861baec6d), C(7334878a00e96e72),
    C(5c8685fee2e4ce55), C(228c49268d6a4345), C(3b04ee2861baec6d), C(7334878a00e96e72),
    C(7317164b2ce711bb), C(e645447e363e8ca1), C(d326d129ad7b4e7f), C(58b9b76d5c2eb272)
  },
  {
    C(54e4d0cab7ec5c27), C(31ca61d2262a9acc), C(30bd3a50d8082ff6), C(46b3b963bf7e2847), C(b319d04e16ad10b0), C(76c8dd82e6f5a0eb), C(2070363cefb488bc),
    C(46b3b963bf7e2847), C(b319d04e16ad10b0), C(76c8dd82e6f5a0eb), C(2070363cefb488bc),
    C(6f9dbacb2bdc556d), C(88a5fb0b293c1e22), C(cb131d9b9abd84b7), C(21db6f0e147a0040)
  },
  {
    C(882a598e98cf5416), C(36c8dca4a80d9788), C(c386480f07591cfe), C(5b517bcf2005fd9c), C(b9b8f8e5f90e7025), C(2a833e6199e21708), C(bcb7549de5fda812),
    C(5b517bcf2005fd9c), C(b9b8f8e5f90e7025), C(2a833e6199e21708), C(bcb7549de5fda812),
    C(44fc96a3cafa1c34), C(fb7724d4899ec7c7), C(4662e3b87df93e13), C(bcf22545acbcfd4e)
  },
  {
    C(7c37a5376c056d55), C(e0cce8936a06b6f6), C(d32f933fdbec4c7d), C(7ac50423e2be4703), C(546d4b42340d6dc7), C(624f56ee027f12bf), C(5f7f65d1e90c30f9),
    C(7ac50423e2be4703), C(546d4b42340d6dc7), C(624f56ee027f12bf), C(5f7f65d1e90c30f9),
    C(d6f15c19625d2621), C(c7afd12394f24b50), C(2c6adde5d249bcd0), C(6c857e6aa07b9fd2)
  },
  {
    C(21c5e9616f24be97), C(ba3536c86e4b6fe9), C(6d3a65cfe3a9ae06), C(2113903ebd760a31), C(e561f76a5eac8beb), C(86b5b3e76392e166), C(68c8004ccc53e049),
    C(2113903ebd760a31), C(e561f76a5eac8beb), C(86b5b3e76392e166), C(68c8004ccc53e049),
    C(b51a28fe4251dd79), C(fd9c2d4d2a84c3c7), C(5bf2ec8a470d2553), C(135a52cdc76241c9)
  },
  {
    C(a6eaefe74fa7d62b), C(cb34669c751b10eb), C(80da952ad8abd5f3), C(3368262b0e172d82), C(1d51f6c982476285), C(4497675ac57228a9), C(2a71766a71d0b83f),
    C(3368262b0e172d82), C(1d51f6c982476285), C(4497675ac57228a9), C(2a71766a71d0b83f),
    C(79ad94d1e9c1dedd), C(cbf1a1c9f23bfa40), C(3ebf24e068cd638b), C(be8e63472edfb462)
  },
  {
    C(764af88ed4b0b828), C(36946775f20457ce), C(d4bc88ac8281c22e), C(3b2104d68dd9ac02), C(2eca14fcdc0892d0), C(7913b0c09329cd47), C(9373f458938688c8),
    C(3b2104d68dd9ac02), C(2eca14fcdc0892d0), C(7913b0c09329cd47), C(9373f458938688c8),
    C(b4448f52a5bf9425), C(9f8c8b90b61ed532), C(78f6774f48e72961), C(e47c00bf9c1206f4)
  },
  {
    C(5f55a694fb173ea3), C(7db02b80ef5a918b), C(d87ff079f476ca3a), C(1d11117374e0da3), C(744bfbde42106439), C(93a99fab10bb1789), C(246ba292a85d8d7c),
    C(1d11117374e0da3), C(744bfbde42106439), C(93a99fab10bb1789), C(246ba292a85d8d7c),
    C(e5bd7838e9edd53a), C(d9c0b104c79d9019), C(ee3dcc7a8e565de5), C(619c9e0a9cf3596d)
  },
  {
    C(86d086738b0a7701), C(d2402313a4280dda), C(b327aa1a25278366), C(49efdde5d1f98163), C(cbcffcee90f22824), C(951aec1daeb79bab), C(7055e2c70d2eeb4c),
    C(49efdde5d1f98163), C(cbcffcee90f22824), C(951aec1daeb79bab), C(7055e2c70d2eeb4c),
    C(1fc0de9399bacb96), C(dab7bbe67901959e), C(375805eccf683ef0), C(bbb6f465c4bae04e)
  },
  {
    C(acfc8be97115847b), C(c8f0d887bf8d9d1), C(e698fbc6d39bf837), C(61fd1d6b13c1ea77), C(527ed97ff4ae24f0), C(af51a9ebb322c0), C(14f7c25058864825),
    C(61fd1d6b13c1ea77), C(527ed97ff4ae24f0), C(af51a9ebb322c0), C(14f7c25058864825),
    C(f40b2bbeaf9f021d), C(80d827160dfdc2d2), C(77baea2e3650486e), C(5de2d256740a1a97)
  },
  {
    C(dc5ad3c016024d4), C(a0235e954da1a152), C(6daa8a4ed194cc43), C(185e650afc8d39f8), C(adba03a4d40de998), C(9975c776b499b26f), C(9770c59368a43a2),
    C(185e650afc8d39f8), C(adba03a4d40de998), C(9975c776b499b26f), C(9770c59368a43a2),
    C(d2776f0cf0e4f66c), C(38eaaabfb743f7f6), C(c066f03d959b3f07), C(9d91c2d52240d546)
  },
  {
    C(a0e91182f03277f7), C(15c6ebef7376556), C(516f887657ab5a), C(f95050524c7f4b84), C(460dcebbaaa09ae3), C(a9f7a9f0b1b2a961), C(5f8dc5e198e34539),
    C(f95050524c7f4b84), C(460dcebbaaa09ae3), C(a9f7a9f0b1b2a961), C(5f8dc5e198e34539),
    C(9c49227ffcff07cb), C(a29388e9fcb794c8), C(475867910d110cba), C(8c9a5cee480b5bac)
  },
  {
    C(767f1dbd1dba673b), C(1e466a3848a5b01e), C(483eadef1347cd6e), C(a67645c72f54fe24), C(c7a5562c69bd796b), C(e14201a35b55e4a6), C(b3a6d89f19d8f774),
    C(a67645c72f54fe24), C(c7a5562c69bd796b), C(e14201a35b55e4a6), C(b3a6d89f19d8f774),
    C(bb4d607ac22bebe5), C(792030edeaa924e0), C(138730dcb60f7e32), C(699d9dcc326c72dc)
  },
  {
    C(a5e30221500dcd53), C(3a1058d71c9fad93), C(510520710c6444e8), C(a6a5e60c2c1d0108), C(45c8ea4e14bf8c6b), C(213a7235416b86df), C(c186072f80d56ad3),
    C(a6a5e60c2c1d0108), C(45c8ea4e14bf8c6b), C(213a7235416b86df), C(c186072f80d56ad3),
    C(2e7be098db59d832), C(d5fa382f3717a0ee), C(b168b26921d243d), C(61601a60c2addfbb)
  },
  {
    C(ebaed82e48e18ce4), C(cfe6836b65ebe7c7), C(504d9d388684d449), C(bd9c744ee9e3308e), C(faefbb8d296b65d4), C(eba051fe2404c25f), C(250c8510b8931f87),
    C(bd9c744ee9e3308e), C(faefbb8d296b65d4), C(eba051fe2404c25f), C(250c8510b8931f87),
    C(3c4a49150dc5676f), C(6c28793c565345c4), C(9df6dd8829a6d8fb), C(760d3a023fab72e7)
  },
  {
    C(ffa50913362b118d), C(626d52251a8ec3e0), C(76ce4b9dde2e8c5e), C(fc57418d92e52355), C(6b46c559e67a063), C(3f5c269e10690c5c), C(6870de8d49e65349),
    C(fc57418d92e52355), C(6b46c559e67a063), C(3f5c269e10690c5c), C(6870de8d49e65349),
    C(88737e5c672de296), C(ca71fca5f4c4f1ce), C(42fca3fa7f60e031), C(4a70246d0d4c2bd8)
  },
  {
    C(256186bcda057f54), C(fb059b012049fd8e), C(304e07418b5f739b), C(3e166f9fac2eec0b), C(82bc11707ec4a7a4), C(e29acd3851ce36b6), C(9765ca9323d30046),
    C(3e166f9fac2eec0b), C(82bc11707ec4a7a4), C(e29acd3851ce36b6), C(9765ca9323d30046),
    C(dab63e7790017f7c), C(b9559988bff0f170), C(48d9ef8aea13eee8), C(e31e47857c511ec2)
  },
  {
    C(382b15315e84f28b), C(f9a2578b79590b72), C(708936af6d4450e8), C(76a9d4843df75c1c), C(2c33447da3f2c70a), C(5e4dcf2eaeace0d6), C(2ae1727aa7220634),
    C(76a9d4843df75c1c), C(2c33447da3f2c70a), C(5e4dcf2eaeace0d6), C(2ae1727aa7220634),
    C(a122f6b52e1130ba), C(a17ae9a21f345e91), C(ff67313f1d0906a9), C(bb16dc0acd6ebecc)
  },
  {
    C(9983a9cc5576d967), C(29e37689a173109f), C(c526073a91f2808c), C(fe9a9d4a799cf817), C(7ca841999012c0d1), C(8b3abfa4bd2aa28e), C(4ed49274526602eb),
    C(fe9a9d4a799cf817), C(7ca841999012c0d1), C(8b3abfa4bd2aa28e), C(4ed49274526602eb),
    C(40995df99063fe23), C(7f51b7ceded05144), C(743c89732b265bf2), C(10c8e1fd835713fd)
  },
  {
    C(c2c58a843f733bdb), C(516c47c97b4ba886), C(abc3cae0339517db), C(be29af0dad5c9d27), C(70f802599d97fe08), C(23af3f67d941e52b), C(a031edd8b3a008fb),
    C(be29af0dad5c9d27), C(70f802599d97fe08), C(23af3f67d941e52b), C(a031edd8b3a008fb),
    C(43431336b198f8fd), C(7c4b60284e1c2245), C(51ee580ddabae1b3), C(ca99bd13845d8f7f)
  },
  {
    C(648ff27fabf93521), C(d7fba33cbc153035), C(3dbcdcf87ad06c9e), C(52ddbdc9dfd26990), C(d46784cd2aeabb28), C(bd3a15e5e4eb7177), C(b5d7632e19a2cd),
    C(52ddbdc9dfd26990), C(d46784cd2aeabb28), C(bd3a15e5e4eb7177), C(b5d7632e19a2cd),
    C(8007450fa355dc04), C(41ca59f64588bb5c), C(66f2ca6b7487499d), C(8098716530db9bea)
  },
  {
    C(99be55475dcb3461), C(d94ffa462f6ba8dc), C(dbab2b456bdf13bb), C(f28f496e15914b2d), C(1171ce20f49cc87d), C(1b5f514bc1b377a9), C(8a02cb12ec4d6397),
    C(f28f496e15914b2d), C(1171ce20f49cc87d), C(1b5f514bc1b377a9), C(8a02cb12ec4d6397),
    C(1c6540740c128d79), C(d085b67114969f41), C(af8c1988085306f3), C(4681f415d9ce8038)
  },
  {
    C(e16fbb9303dd6d92), C(4d92b99dd164db74), C(3f98f2c9da4f5ce3), C(c65b38c5a47eeed0), C(5c5301c8ee3923a6), C(51bf9f9eddec630e), C(b1cbf1a68be455c2),
    C(c65b38c5a47eeed0), C(5c5301c8ee3923a6), C(51bf9f9eddec630e), C(b1cbf1a68be455c2),
    C(c356f5f98499bdb8), C(d897df1ad63fc1d4), C(9bf2a3a69982e93a), C(a2380d43e271bcc8)
  },
  {
    C(4a57a4899834e4c0), C(836c4df2aac32257), C(cdb66b29e3e12147), C(c734232cbda1eb4c), C(30a3cffff6b9dda0), C(d199313e17cca1ed), C(594d99e4c1360d82),
    C(c734232cbda1eb4c), C(30a3cffff6b9dda0), C(d199313e17cca1ed), C(594d99e4c1360d82),
    C(ccc37662829a65b7), C(cae30ff4d2343ce9), C(54da907f7aade4fa), C(5d6e4a0272958922)
  },
  {
    C(f658958cdf49f149), C(de8e4a622b7a16b), C(a227ebf448c80415), C(3de9e38b3a369785), C(84d160d688c573a9), C(8f562593add0ad54), C(4446b762cc34e6bf),
    C(3de9e38b3a369785), C(84d160d688c573a9), C(8f562593add0ad54), C(4446b762cc34e6bf),
    C(2f795f1594c7d598), C(29e05bd1e0dceaff), C(a9a88f2962b49589), C(4b9c86c141ac120b)
  },
  {
    C(ae1befc65d3ea04d), C(cfd9bc0388c8fd00), C(522f2e1f6cdb31af), C(585447ebe078801a), C(14a31676ec4a2cbd), C(b274e7e6af86a5e1), C(2d487019570bedce),
    C(585447ebe078801a), C(14a31676ec4a2cbd), C(b274e7e6af86a5e1), C(2d487019570bedce),
    C(ea1dc9ef3c7b2fcc), C(bde99d4af2f4ee8c), C(64e4c43cd7c43442), C(9b5262ee2eed2f99)
  },
  {
    C(2fc8f9fc5946296d), C(6a2b94c6765ebfa2), C(f4108b8c79662fd8), C(3a48de4a1e994623), C(6318e6e1ff7bc092), C(84aee2ea26a048fb), C(cf3c393fdad7b184),
    C(3a48de4a1e994623), C(6318e6e1ff7bc092), C(84aee2ea26a048fb), C(cf3c393fdad7b184),
    C(28b265bd8985a71e), C(bd3d97dbd76d89a5), C(b04ba1623c0937d), C(b6de821229693515)
  },
  {
    C(efdb4dc26e84dce4), C(9ce45b6172dffee8), C(c15ad8c8bcaced19), C(f10cc2bcf0475411), C(1126f457c160d8f5), C(34c67f6ea249d5cc), C(3ab7633f4557083),
    C(f10cc2bcf0475411), C(1126f457c160d8f5), C(34c67f6ea249d5cc), C(3ab7633f4557083),
    C(3b2e4d8611a03bd7), C(3103d6e63d71c3c9), C(43a56a0b93bb9d53), C(50aa3ae25803c403)
  },
  {
    C(e84a123b3e1b0c91), C(735cc1d493c5e524), C(287030af8f4ac951), C(fb46abaf4713dda0), C(e8835b9a08cf8cb2), C(3b85a40e6bee4cce), C(eea02a3930757200),
    C(fb46abaf4713dda0), C(e8835b9a08cf8cb2), C(3b85a40e6bee4cce), C(eea02a3930757200),
    C(fe7057d5fb18ee87), C(723d258b36eada2a), C(67641393692a716c), C(c8539a48dae2e539)
  },
  {
    C(686c22d2863c48a6), C(1ee6804e3ddde627), C(8d66184dd34ddac8), C(35ac1bc76c11976), C(fed58f898503280d), C(ab6fcb01c630071e), C(edabf3ec7663c3c9),
    C(35ac1bc76c11976), C(fed58f898503280d), C(ab6fcb01c630071e), C(edabf3ec7663c3c9),
    C(591ec5025592b76e), C(918a77179b072163), C(25421d9db4c81e1a), C(96f1b3be51f0b548)
  },
  {
    C(2c5c1c9fa0ecfde0), C(266a71b430afaec3), C(53ab2d731bd8184a), C(5722f16b15e7f206), C(35bb5922c0946610), C(b8d72c08f927f2aa), C(65f2c378cb9e8c51),
    C(5722f16b15e7f206), C(35bb5922c0946610), C(b8d72c08f927f2aa), C(65f2c378cb9e8c51),
    C(cd42fec772c2d221), C(10ccd5d7bacffdd9), C(a75ecb52192f60e2), C(a648f5fe45e5c164)
  },
  {
    C(7a0ac8dd441c9a9d), C(4a4315964b7377f0), C(24092991c8f27459), C(9c6868d561691eb6), C(78b7016996f98828), C(651e072f06c9e7b7), C(fed953d1251ae90),
    C(9c6868d561691eb6), C(78b7016996f98828), C(651e072f06c9e7b7), C(fed953d1251ae90),
    C(7a4d19fdd89e368c), C(d8224d83b6b9a753), C(3a93520a455ee9c9), C(159942bea42b999c)
  },
  {
    C(c6f9a31dfc91537c), C(b3a250ae029272f8), C(d065fc76d79ec222), C(d2baa99749c71d52), C(5f90a2cfc2a3f637), C(79e4aca7c8bb0998), C(981633149c85c0ba),
    C(d2baa99749c71d52), C(5f90a2cfc2a3f637), C(79e4aca7c8bb0998), C(981633149c85c0ba),
    C(5ded415df904b2ee), C(d37d1fc032ebca94), C(ed5b024594967bf7), C(ed7ae636d467e961)
  },
  {
    C(2d12010eaf7d8d3d), C(eaec74ccd9b76590), C(541338571d45608b), C(e97454e4191065f3), C(afb357655f2a5d1c), C(521ac1614653c130), C(c8a8cac96aa7f32c),
    C(e97454e4191065f3), C(afb357655f2a5d1c), C(521ac1614653c130), C(c8a8cac96aa7f32c),
    C(196d7f3f386dfd29), C(1dcd2da5227325cc), C(10e3b9fa712d3405), C(fdf7864ede0856c0)
  },
  {
    C(f46de22b2d79a5bd), C(e3e198ba766c0a29), C(828d8c137216b797), C(bafdb732c8a29420), C(2ed0b9f4548a9ac3), C(f1ed2d5417d8d1f7), C(451462f90354d097),
    C(bafdb732c8a29420), C(2ed0b9f4548a9ac3), C(f1ed2d5417d8d1f7), C(451462f90354d097),
    C(bdd091094408851a), C(c4c1731c1ea46c2c), C(615a2348d60409a8), C(fbc2f058d5539bcc)
  },
  {
    C(2ce2f3e89fa141fe), C(ac588fe6ab2b719), C(59b848c80739487d), C(423722957b566d10), C(ae4be02664998dc6), C(64017aacfa69ef80), C(28076dddbf65a40a),
    C(423722957b566d10), C(ae4be02664998dc6), C(64017aacfa69ef80), C(28076dddbf65a40a),
    C(873bc41acb810f94), C(ac0edafb574b7c0d), C(937d5d5fd95330bf), C(4ea91171e208bd7e)
  },
  {
    C(8aa75419d95555dd), C(bdb046419d0bf1b0), C(aadf49f217b153da), C(c3cbbe7eb0f5e126), C(fd1809c329311bf6), C(9c26cc255714d79d), C(67093aeb89f5d8c8),
    C(c3cbbe7eb0f5e126), C(fd1809c329311bf6), C(9c26cc255714d79d), C(67093aeb89f5d8c8),
    C(265954c61009eaf7), C(a5703e8073eaf83f), C(855382b1aed9c128), C(a6652d5a53d4a008)
  },
  {
    C(1fbf19dd9207e6aa), C(722834f3c5e43cb7), C(e3c13578c5a69744), C(db9120bc83472135), C(f3d9f715e669cfd5), C(63facc852f487dda), C(9f08fd85a3a78111),
    C(db9120bc83472135), C(f3d9f715e669cfd5), C(63facc852f487dda), C(9f08fd85a3a78111),
    C(6c1e5c694b51b7ca), C(bbceb2e47d44f6a1), C(2eb472efe06f8330), C(1844408e2bb87ee)
  },
  {
    C(6f11f9c1131f1182), C(6f90740debc7bad2), C(8d6e4e2d46ee614b), C(403e3793f0805ac3), C(6278da3d8667a055), C(98eceadb4f237978), C(4daa96284c847b0),
    C(403e3793f0805ac3), C(6278da3d8667a055), C(98eceadb4f237978), C(4daa96284c847b0),
    C(ab119ac9f803d770), C(ab893fe847208376), C(f9d9968ae4472ac3), C(b149ff3b35874201)
  },
  {
    C(92e896d8bfdebdb5), C(2d5c691a0acaeba7), C(377d7f86b7cb2f8b), C(b8a0738135dde772), C(57fb6c9033fc5f35), C(20e628f266e63e1), C(1ad6647eaaa153a3),
    C(b8a0738135dde772), C(57fb6c9033fc5f35), C(20e628f266e63e1), C(1ad6647eaaa153a3),
    C(10005c85a89e601a), C(cc9088ed03a78e4a), C(c8d3049b8c0d26a1), C(26e8c0e936cf8cce)
  },
  {
    C(369ba54df3c534d1), C(972c7d2be5f62834), C(112c8d0cfcc8b1e), C(bcddd22a14192678), C(446cf170a4f05e72), C(c9e992c7a79ce219), C(fa4762e60a93cf84),
    C(bcddd22a14192678), C(446cf170a4f05e72), C(c9e992c7a79ce219), C(fa4762e60a93cf84),
    C(b2e11a375a352f), C(a70467d0fd624cf1), C(776b638246febf88), C(e7d1033f7faa39b5)
  },
  {
    C(bcc4229e083e940e), C(7a42ebe9e8f526b5), C(bb8d1f389b0769ee), C(ae6790e9fe24c57a), C(659a16feab53eb5), C(6fd4cfade750bf16), C(31b1acd328815c81),
    C(ae6790e9fe24c57a), C(659a16feab53eb5), C(6fd4cfade750bf16), C(31b1acd328815c81),
    C(8a711090a6ccfd44), C(363240c31681b80e), C(ad791f19de0b07e9), C(d512217d21c7c370)
  },
  {
    C(17c648f416fb15ca), C(fe4d070d14d71a1d), C(ff22eac66f7eb0d3), C(fa4c10f92facc6c7), C(94cad9e4daecfd58), C(6ffcf829a275d7ef), C(2a35d2436894d549),
    C(fa4c10f92facc6c7), C(94cad9e4daecfd58), C(6ffcf829a275d7ef), C(2a35d2436894d549),
    C(c9ea25549513f5a), C(93f7cf06df2d0206), C(ef0da319d38fe57c), C(f715dc84df4f4a75)
  },
  {
    C(8b752dfa2f9fa592), C(ca95e87b662fe94d), C(34da3aadfa49936d), C(bf1696df6e61f235), C(9724fac2c03e3859), C(d9fd1463b07a8b61), C(f8e397251053d8ca),
    C(bf1696df6e61f235), C(9724fac2c03e3859), C(d9fd1463b07a8b61), C(f8e397251053d8ca),
    C(c6d26d868c9e858e), C(2f4a1cb842ed6105), C(6cc48927bd59d1c9), C(469e836d0b7901e1)
  },
  {
    C(3edda5262a7869bf), C(a15eab8c522050c9), C(ba0853c48707207b), C(4d751c1a836dcda3), C(9747a6e96f1dd82c), C(3c986fc5c9dc9755), C(a9d04f3a92844ecd),
    C(4d751c1a836dcda3), C(9747a6e96f1dd82c), C(3c986fc5c9dc9755), C(a9d04f3a92844ecd),
    C(2da9c6cede185e36), C(fae575ef03f987d6), C(b4a6a620b2bee11a), C(8acba91c5813c424)
  },
  {
    C(b5776f9ceaf0dba2), C(546eee4cee927b0a), C(ce70d774c7b1cf77), C(7f707785c2d807d7), C(1ea8247d40cdfae9), C(4945806eac060028), C(1a14948790321c37),
    C(7f707785c2d807d7), C(1ea8247d40cdfae9), C(4945806eac060028), C(1a14948790321c37),
    C(ba3327bf0a6ab79e), C(54e2939592862de8), C(b7d4651234fa11c7), C(d122970552454def)
  },
  {
    C(313161f3ce61ec83), C(c6c5acb78303987d), C(f00761c6c6e44cee), C(ea660b39d2528951), C(e84537f81a44826a), C(b850bbb69593c26d), C(22499793145e1209),
    C(ea660b39d2528951), C(e84537f81a44826a), C(b850bbb69593c26d), C(22499793145e1209),
    C(4c61b993560bbd58), C(636d296abe771743), C(f1861b17b8bc3146), C(cd5fca4649d30f8a)
  },
  {
    C(6e23080c57f4bcb), C(5f4dad6078644535), C(f1591bc445804407), C(46ca76959d0d4824), C(200b16bb4031e6a5), C(3d0e4718ed5363d2), C(4c8cfcc96382106f),
    C(46ca76959d0d4824), C(200b16bb4031e6a5), C(3d0e4718ed5363d2), C(4c8cfcc96382106f),
    C(8d6258d795b8097b), C(23ae7cd1cab4b141), C(cbe74e8fd420afa), C(d553da4575629c63)
  },
  {
    C(a194c120f440fd48), C(ac0d985eef446947), C(5df9fa7d97244438), C(fce2269035535eba), C(2d9b4b2010a90960), C(2b0952b893dd72f0), C(9a51e8462c1111de),
    C(fce2269035535eba), C(2d9b4b2010a90960), C(2b0952b893dd72f0), C(9a51e8462c1111de),
    C(8682b5e0624432a4), C(de8500edda7c67a9), C(4821b171f562c5a2), C(ecb17dea1002e2df)
  },
  {
    C(3c78f67ee87b62fe), C(274c83c73f20f662), C(25a94c36d3763332), C(7e053f1b873bed61), C(d1c343547cd9c816), C(4deee69b90a52394), C(14038f0f3128ca46),
    C(7e053f1b873bed61), C(d1c343547cd9c816), C(4deee69b90a52394), C(14038f0f3128ca46),
    C(ebbf836e38c70747), C(c3c1077b9a7598d0), C(e73c720a27b07ba7), C(ec57f8a9a75af4d9)
  },
  {
    C(b7d2aee81871e3ac), C(872ac6546cc94ff2), C(a1b0d2f507ad2d8f), C(bdd983653b339252), C(c02783d47ab815f8), C(36c5dc27d64d776c), C(5193988eea7df808),
    C(bdd983653b339252), C(c02783d47ab815f8), C(36c5dc27d64d776c), C(5193988eea7df808),
    C(8d8cca9c605cdb4a), C(334904fd32a1f934), C(dbfc15742057a47f), C(f3f92db42ec0cba1)
  },
  {
    C(41ec0382933e8f72), C(bd5e52d651bf3a41), C(cbf51a6873d4b29e), C(1c8c650bfed2c546), C(9c9085c070350c27), C(e82305be3bded854), C(cf56326bab3d685d),
    C(1c8c650bfed2c546), C(9c9085c070350c27), C(e82305be3bded854), C(cf56326bab3d685d),
    C(f94db129adc6cecc), C(1f80871ec4b35deb), C(c0dc1a4c74d63d0), C(d3cac509f998c174)
  },
  {
    C(7fe4e777602797f0), C(626e62f39f7c575d), C(d15d6185215fee2f), C(f82ef80641514b70), C(e2702de53389d34e), C(9950592b7f2da8d8), C(d6b960bf3503f893),
    C(f82ef80641514b70), C(e2702de53389d34e), C(9950592b7f2da8d8), C(d6b960bf3503f893),
    C(95de69e4f131a9b), C(ee6f56eeff9cdefa), C(28f4f86c2b856b72), C(b73d2decaac56b5b)
  },
  {
    C(aa71127fd91bd68a), C(960f6304500f8069), C(5cfa9758933beba8), C(dcbbdeb1f56b0ac5), C(45164c603d084ce4), C(85693f4ef7e34314), C(e3a3e3a5ec1f6252),
    C(dcbbdeb1f56b0ac5), C(45164c603d084ce4), C(85693f4ef7e34314), C(e3a3e3a5ec1f6252),
    C(91f4711c59532bab), C(5e5a61d26f97200b), C(ffa65a1a41da5883), C(5f0e712235371eef)
  },
  {
    C(677b53782a8af152), C(90d76ef694361f72), C(fa2cb9714617a9e0), C(72c8667cc1e45aa9), C(3a0aa035bbcd1ef6), C(588e89b034fde91b), C(f62e4e1d81c1687),
    C(72c8667cc1e45aa9), C(3a0aa035bbcd1ef6), C(588e89b034fde91b), C(f62e4e1d81c1687),
    C(1ea81508efa11e09), C(1cf493a4dcd49aad), C(8217d0fbe8226130), C(607b979c0eb297dd)
  },
  {
    C(8f97bb03473c860f), C(e23e420f9a32e4a2), C(3432c97895fea7cf), C(69cc85dac0991c6c), C(4a6c529f94e9c36a), C(e5865f8da8c887df), C(27e8c77da38582e0),
    C(69cc85dac0991c6c), C(4a6c529f94e9c36a), C(e5865f8da8c887df), C(27e8c77da38582e0),
    C(8e60596b4e327dbc), C(955cf21baa1ddb18), C(c24a8eb9360370aa), C(70d75fd116c2cab1)
  },
  {
    C(fe50ea9f58e4de6f), C(f0a085b814230ce7), C(89407f0548f90e9d), C(6c595ea139648eba), C(efe867c726ab2974), C(26f48ecc1c3821cf), C(55c63c1b3d0f1549),
    C(6c595ea139648eba), C(efe867c726ab2974), C(26f48ecc1c3821cf), C(55c63c1b3d0f1549),
    C(552e5f78e1d87a69), C(c9bfe2747a4eedf0), C(d5230acb6ef95a1), C(1e812f3c0d9962bd)
  },
  {
    C(56eb0fcb9852bd27), C(c817b9a578c7b12), C(45427842795bfa84), C(8dccc5f52a65030c), C(f89ffa1f4fab979), C(7d94da4a61305982), C(1ba6839d59f1a07a),
    C(8dccc5f52a65030c), C(f89ffa1f4fab979), C(7d94da4a61305982), C(1ba6839d59f1a07a),
    C(e0162ec1f40d583e), C(6abf0b85552c7c33), C(f14bb021a875867d), C(c12a569c8bfe3ba7)
  },
  {
    C(6be2903d8f07af90), C(26aaf7b795987ae8), C(44a19337cb53fdeb), C(f0e14afc59e29a3a), C(a4d0084172a98c0d), C(275998a345d04f0f), C(db73704d81680e8d),
    C(f0e14afc59e29a3a), C(a4d0084172a98c0d), C(275998a345d04f0f), C(db73704d81680e8d),
    C(351388cf7529b1b1), C(a3155d0237571da5), C(355231b516da2890), C(263c5a3d498c1cc)
  },
  {
    C(58668066da6bfc4), C(a4ea2eb7212df3dd), C(481f64f7ca220524), C(11b3b649b1cea339), C(57f4ad5b54d71118), C(feeb30bec803ab49), C(6ed9bcc1973d9bf9),
    C(11b3b649b1cea339), C(57f4ad5b54d71118), C(feeb30bec803ab49), C(6ed9bcc1973d9bf9),
    C(bf2859d9964a70c8), C(d31ab162ca25f24e), C(70349336ff55d5d5), C(9a2fa97115ef4409)
  },
  {
    C(2d04d1fbab341106), C(efe0c5b2878b444c), C(882a2a889b5e8e71), C(18cc96be09e5455), C(1ad58fd26919e409), C(76593521c4a0006b), C(f1361f348fa7cbfb),
    C(18cc96be09e5455), C(1ad58fd26919e409), C(76593521c4a0006b), C(f1361f348fa7cbfb),
    C(205bc68e660b0560), C(74360e11f9fc367e), C(a88b7b0fa86caf), C(a982d749b30d4e4c)
  },
  {
    C(d366b37bcd83805b), C(a6d16fea50466886), C(cb76dfa8eaf74d70), C(389c44e423749aa), C(a30d802bec4e5430), C(9ac1279f92bea800), C(686ef471c2624025),
    C(389c44e423749aa), C(a30d802bec4e5430), C(9ac1279f92bea800), C(686ef471c2624025),
    C(2c21a72f8e3a3423), C(df5ab83f0918646a), C(cd876e0cb4df80fa), C(5abbb92679b3ea36)
  },
  {
    C(bbb9bc819ab65946), C(25e0c756c95803e2), C(82a73a1e1cc9bf6a), C(671b931b702519a3), C(61609e7dc0dd9488), C(9cb329b8cab5420), C(3c64f8ea340096ca),
    C(671b931b702519a3), C(61609e7dc0dd9488), C(9cb329b8cab5420), C(3c64f8ea340096ca),
    C(1690afe3befd3afb), C(4d3c18a846602740), C(a6783133a31dd64d), C(ecf4665e6bc76729)
  },
  {
    C(8e994eac99bbc61), C(84de870b6f3c114e), C(150efc95ce7b0cd2), C(4c5d48abf41185e3), C(86049a83c7cdcc70), C(ad828ff609277b93), C(f60fe028d582ccc7),
    C(4c5d48abf41185e3), C(86049a83c7cdcc70), C(ad828ff609277b93), C(f60fe028d582ccc7),
    C(464e0b174da0cbd4), C(eadf1df69041b06e), C(48cb9c96a9df1cdc), C(b7e5ee62809223a1)
  },
  {
    C(364cabf6585e2f7d), C(3be1cc452509807e), C(1236ce85788680d4), C(4cea77c54fc3583a), C(9a2a64766fd77614), C(63e6c9254b5dc4db), C(26af12ba3bf5988e),
    C(4cea77c54fc3583a), C(9a2a64766fd77614), C(63e6c9254b5dc4db), C(26af12ba3bf5988e),
    C(4a821aca3ffa26a1), C(99aa9aacbb3d08e3), C(619ac77b52e8a823), C(68c745a1ce4b7adb)
  },
  {
    C(e878e2200893d775), C(76b1e0a25867a803), C(9c14d6d91f5ae2c5), C(ac0ffd8d64e242ed), C(e1673ee2dd997587), C(8cdf3e9369d61003), C(c37c9a5258b98eba),
    C(ac0ffd8d64e242ed), C(e1673ee2dd997587), C(8cdf3e9369d61003), C(c37c9a5258b98eba),
    C(f252b2e7b67dd012), C(47fc1eb088858f28), C(59c42e4af1353223), C(e05b6c61c19eb26e)
  },
  {
    C(6f6a014b9a861926), C(269e13a120277867), C(37fc8a181e78711b), C(33dd054c41f3aef2), C(4fc8ab1a2ef3da7b), C(597178c3756a06dc), C(748f8aadc540116f),
    C(33dd054c41f3aef2), C(4fc8ab1a2ef3da7b), C(597178c3756a06dc), C(748f8aadc540116f),
    C(78e3be34de99461e), C(28b7b60d90dddab4), C(e47475fa9327a619), C(88b17629e6265924)
  },
  {
    C(da52b64212e8149b), C(121e713c1692086f), C(f3d63cfa03850a02), C(f0d82bafec3c564c), C(37dece35b549a1ce), C(5fb28f6078c4a2bd), C(b69990b7d9405710),
    C(f0d82bafec3c564c), C(37dece35b549a1ce), C(5fb28f6078c4a2bd), C(b69990b7d9405710),
    C(3af5223132071100), C(56d5bb35f3bb5d2a), C(fcad4a4d5d3a1bc7), C(f17bf3d8853724d0)
  },
  {
    C(1100f797ce53a629), C(f528c6614a1a30c2), C(30e49fb56bec67fa), C(f991664844003cf5), C(d54f5f6c8c7cf835), C(ca9cc4437c591ef3), C(d5871c77cf8fb424),
    C(f991664844003cf5), C(d54f5f6c8c7cf835), C(ca9cc4437c591ef3), C(d5871c77cf8fb424),
    C(5cf90f1e617b750c), C(1648f825ab986232), C(936cf225126a60), C(90fa5311d6f2445c)
  },
  {
    C(4f00655b76e9cfda), C(9dc5c707772ed283), C(b0f885f1e01927ec), C(6e4d6843289dfb47), C(357b41c6e5fd561f), C(491e386bacb6df3c), C(86be1b64ecd9945c),
    C(6e4d6843289dfb47), C(357b41c6e5fd561f), C(491e386bacb6df3c), C(86be1b64ecd9945c),
    C(be9547e3cfd85fae), C(f9e26ac346b430a8), C(38508b84b0e68cff), C(a28d49dbd5562703)
  },
  {
    C(d970198b6ca854db), C(92e3d1786ae556a0), C(99a165d7f0d85cf1), C(6548910c5f668397), C(a5c8d20873e7de65), C(5b7c4ecfb8e38e81), C(6aa50a5531dad63e),
    C(6548910c5f668397), C(a5c8d20873e7de65), C(5b7c4ecfb8e38e81), C(6aa50a5531dad63e),
    C(ab903d724449e003), C(ea3cc836c28fef88), C(4b250d6c7200949d), C(13a110654fa916c0)
  },
  {
    C(76c850754f28803), C(a4bffed2982cb821), C(6710e352247caf63), C(d9cbf5b9c31d964e), C(25c8f890178b97ae), C(e7c46064676cde9f), C(d8bb5eeb49c06336),
    C(d9cbf5b9c31d964e), C(25c8f890178b97ae), C(e7c46064676cde9f), C(d8bb5eeb49c06336),
    C(962b35ae89d5f4c1), C(c49083801ac2c21), C(2db46ddec36ff33b), C(da48992ab8da284)
  },
  {
    C(9c98da9763f0d691), C(f5437139a3d40401), C(6f493c26c42f91e2), C(e857e4ab2d124d5), C(6417bb2f363f36da), C(adc36c9c92193bb1), C(d35bd456172df3df),
    C(e857e4ab2d124d5), C(6417bb2f363f36da), C(adc36c9c92193bb1), C(d35bd456172df3df),
    C(577da94064d3a3d6), C(23f13d7532ea496a), C(6e09392d80b8e85b), C(2e05ff6f23663892)
  },
  {
    C(22f8f6869a5f325), C(a0e7a96180772c26), C(cb71ea6825fa3b77), C(39d3dec4e718e903), C(900c9fbdf1ae2428), C(305301da2584818), C(c6831f674e1fdb1f),
    C(39d3dec4e718e903), C(900c9fbdf1ae2428), C(305301da2584818), C(c6831f674e1fdb1f),
    C(8ad0e38ffe71babf), C(554ac85a8a837e64), C(9900c582cf401356), C(169f646b01ed7762)
  },
  {
    C(9ae7575fc14256bb), C(ab9c5a397fabc1b3), C(1d3f582aaa724b2e), C(94412f598ef156), C(15bf1a588f25b327), C(5756646bd68ce022), C(f062a7d29be259a5),
    C(94412f598ef156), C(15bf1a588f25b327), C(5756646bd68ce022), C(f062a7d29be259a5),
    C(aa99c683cfb60b26), C(9e3b7d4b17f91273), C(301d3f5422dd34cf), C(53d3769127253551)
  },
  {
    C(540040e79752b619), C(670327e237c88cb3), C(50962f261bcc31d9), C(9a8ea2b68b2847ec), C(bc24ab7d4cbbda31), C(df5aff1cd42a9b57), C(db47d368295f4628),
    C(9a8ea2b68b2847ec), C(bc24ab7d4cbbda31), C(df5aff1cd42a9b57), C(db47d368295f4628),
    C(9a66c221d1bf3f3), C(7ae74ee1281de8ee), C(a4e173e2c787621f), C(5b51062d10ae472)
  },
  {
    C(34cbf85722d897b1), C(6208cb2a0fff4eba), C(e926cbc7e86f544e), C(883706c4321efee0), C(8fd5d3d84c7827e4), C(a5c80e455a7ccaaa), C(3515f41164654591),
    C(883706c4321efee0), C(8fd5d3d84c7827e4), C(a5c80e455a7ccaaa), C(3515f41164654591),
    C(2c08bfc75dbfd261), C(6e9eadf14f8c965e), C(18783f5770cd19a3), C(a6c7f2f1aa7b59ea)
  },
  {
    C(46afa66366bf5989), C(aa0d424ac649008b), C(97a9108b3cd9c5c9), C(6ca08e09227a9630), C(8b11f73a8e5b80eb), C(2391bb535dc7ce02), C(e43e2529cf36f4b9),
    C(6ca08e09227a9630), C(8b11f73a8e5b80eb), C(2391bb535dc7ce02), C(e43e2529cf36f4b9),
    C(c9bd6d82b7a73d9d), C(b2ed9bae888447ac), C(bd22bb13af0cd06d), C(62781441785b355b)
  },
  {
    C(e15074b077c6e560), C(7c8f2173fcc34afa), C(8aad55bc3bd38370), C(d407ecdbfb7cb138), C(642442eff44578af), C(d3e9fdaf71a5b79e), C(c87c53eda46aa860),
    C(d407ecdbfb7cb138), C(642442eff44578af), C(d3e9fdaf71a5b79e), C(c87c53eda46aa860),
    C(8462310a2c76ff51), C(1bc17a2e0976665e), C(6ec446b13b4d79cf), C(388c7a904b4264c1)
  },
  {
    C(9740b2b2d6d06c6), C(e738265f9de8dafc), C(fdc947c1fca8be9e), C(d6936b41687c1e3d), C(a1a2deb673345994), C(91501e58b17168bd), C(b8edee2b0b708dfc),
    C(d6936b41687c1e3d), C(a1a2deb673345994), C(91501e58b17168bd), C(b8edee2b0b708dfc),
    C(ddf4b43dafd17445), C(44015d050a04ce5c), C(1019fd9ab82c4655), C(c803aea0957bcdd1)
  },
  {
    C(f1431889f2db1bff), C(85257aa1dc6bd0d0), C(1abbdea0edda5be4), C(775aa89d278f26c3), C(a542d20265e3ef09), C(933bdcac58a33090), C(c43614862666ca42),
    C(775aa89d278f26c3), C(a542d20265e3ef09), C(933bdcac58a33090), C(c43614862666ca42),
    C(4c5e54d481a9748d), C(65ce3cd0db838b26), C(9ccbb4005c7f09d2), C(e6dda9555dde899a)
  },
  {
    C(e2dd273a8d28c52d), C(8cd95915fdcfd96b), C(67c0f5b1025f0699), C(cbc94668d48df4d9), C(7e3d656e49d632d1), C(8329e30cac7a61d4), C(38e6cd1e2034e668),
    C(cbc94668d48df4d9), C(7e3d656e49d632d1), C(8329e30cac7a61d4), C(38e6cd1e2034e668),
    C(41e0bce03ed9394b), C(7be48d0158b9834a), C(9ea8d5d1a976b18b), C(606c424c33617e7a)
  },
  {
    C(e0f79029834cc6ac), C(f2b1dcb87cc5e94c), C(4210bc221fe5e70a), C(fd4a4301d4e2ac67), C(8f84358d25b2999b), C(6c4b7d8a5a22ccbb), C(25df606bb23c9d40),
    C(fd4a4301d4e2ac67), C(8f84358d25b2999b), C(6c4b7d8a5a22ccbb), C(25df606bb23c9d40),
    C(915298b0eaadf85b), C(5ec23cc4c6a74e62), C(d640a4ff99763439), C(1603753fb34ad427)
  },
  {
    C(9dc0a29830bcbec1), C(ec4a01dbd52d96a0), C(cd49c657eff87b05), C(ea487fe948c399e1), C(f5de9b2e59192609), C(4604d9b3248b3a5), C(1929878a22c86a1d),
    C(ea487fe948c399e1), C(f5de9b2e59192609), C(4604d9b3248b3a5), C(1929878a22c86a1d),
    C(3cf6cd7c19dfa1ef), C(46e404ee4af2d726), C(613ab0588a5527b5), C(73e39385ced7e684)
  },
  {
    C(d10b70dde60270a6), C(be0f3b256e23422a), C(6c601297a3739826), C(e327ffc477cd2467), C(ebebba63911f32b2), C(2c2c5c24cf4970a2), C(a3cd2c192c1b8bf),
    C(e327ffc477cd2467), C(ebebba63911f32b2), C(2c2c5c24cf4970a2), C(a3cd2c192c1b8bf),
    C(94cb02c94aaf250b), C(30ca38d5e3dac579), C(d68598a91dc597b5), C(162b050e8de2d92)
  },
  {
    C(58d2459f094d075c), C(b4df247528d23251), C(355283f2128a9e71), C(d046198e4df506c2), C(c61bb9705786ae53), C(b360200380d10da8), C(59942bf009ee7bc),
    C(d046198e4df506c2), C(c61bb9705786ae53), C(b360200380d10da8), C(59942bf009ee7bc),
    C(95806d027f8d245e), C(32df87487ed9d0f4), C(e2c5bc224ce97a98), C(9a47c1e33cfb1cc5)
  },
  {
    C(68c600cdd42d9f65), C(bdf0c331f039ff25), C(1354ac1d98944023), C(b5cdfc0b06fd1bd9), C(71f0ce33b183efab), C(d8ae4f9d4b949755), C(877da19d6424f6b3),
    C(b5cdfc0b06fd1bd9), C(71f0ce33b183efab), C(d8ae4f9d4b949755), C(877da19d6424f6b3),
    C(f7cc5cbf76bc6006), C(c93078f44b98efdb), C(3d482142c727e8bc), C(8e23f92e0616d711)
  },
  {
    C(9fc0bd876cb975da), C(80f41015045d1ade), C(5cbf601fc55c809a), C(7d9c567075001705), C(a2fafeed0df46d5d), C(a70b82990031da8f), C(8611c76abf697e56),
    C(7d9c567075001705), C(a2fafeed0df46d5d), C(a70b82990031da8f), C(8611c76abf697e56),
    C(806911617e1ee53), C(1ce82ae909fba503), C(52df85fea9e404bd), C(dbd184e5d9a11a3e)
  },
  {
    C(7b3e8c267146c361), C(c6ad095af345b726), C(af702ddc731948bd), C(7ca4c883bded44b5), C(c90beb31ee9b699a), C(2cdb4aba3d59b8a3), C(df0d4fa685e938f0),
    C(7ca4c883bded44b5), C(c90beb31ee9b699a), C(2cdb4aba3d59b8a3), C(df0d4fa685e938f0),
    C(cc0e568e91aaa382), C(70ca583a464dbea), C(b7a5859b44710e1a), C(ad141467fdf9a83a)
  },
  {
    C(6c49c6b3c9dd340f), C(897c41d89af37bd1), C(52df69e0e2c68a8d), C(eec4be1f65531a50), C(bf23d928f20f1b50), C(c642009b9c593940), C(c5e59e6ca9e96f85),
    C(eec4be1f65531a50), C(bf23d928f20f1b50), C(c642009b9c593940), C(c5e59e6ca9e96f85),
    C(7fbd53343e7da499), C(dd87e7b88afbd251), C(92696e7683b9f322), C(60ff51ef02c24652)
  },
  {
    C(47324327a4cf1732), C(6044753d211e1dd5), C(1ecae46d75192d3b), C(b6d6315a902807e3), C(ccc8312c1b488e5d), C(b933a7b48a338ec), C(9d6753cd83422074),
    C(b6d6315a902807e3), C(ccc8312c1b488e5d), C(b933a7b48a338ec), C(9d6753cd83422074),
    C(5714bd5c0efdc7a8), C(221585e2c88068ca), C(303342b25678904), C(8c174a03e69a76e)
  },
  {
    C(1e984ef53c5f6aae), C(99ea10dac804298b), C(a3f8c241100fb14d), C(259eb3c63a9c9be6), C(f8991532947c7037), C(a16d20b3fc29cfee), C(493c2e91a775af8c),
    C(259eb3c63a9c9be6), C(f8991532947c7037), C(a16d20b3fc29cfee), C(493c2e91a775af8c),
    C(275fccf4acb08abc), C(d13fb6ea3eeaf070), C(505283e5b702b9ea), C(64c092f9f8df1901)
  },
  {
    C(b88f5c9b8b854cc6), C(54fc5d39825b446), C(a12fc1546eac665d), C(ab90eb7fa58b280c), C(dda26598356aa599), C(64191d63f2586e52), C(cada0075c34e8b02),
    C(ab90eb7fa58b280c), C(dda26598356aa599), C(64191d63f2586e52), C(cada0075c34e8b02),
    C(e7de6532b691d87c), C(a28fec86e368624), C(796c280eebd0241a), C(acfcecb641fdbeee)
  },
  {
    C(9fcb3fdb09e7a63a), C(7a115c9ded150112), C(e9ba629108852f37), C(9b03c7c218c192a), C(93c1dd563f46308e), C(f9553625917ea800), C(e0a52f8a5024c59),
    C(9b03c7c218c192a), C(93c1dd563f46308e), C(f9553625917ea800), C(e0a52f8a5024c59),
    C(2bb3a9e8b053e490), C(8b97936723cd8ff6), C(bf3f835246d02722), C(c8e033da88ecd724)
  },
  {
    C(d58438d62089243), C(d8c19375b228e9d3), C(13042546ed96e790), C(4a42ef343514138c), C(549e62449e225cf1), C(dd8260e2808f68e8), C(69580fc81fcf281b),
    C(4a42ef343514138c), C(549e62449e225cf1), C(dd8260e2808f68e8), C(69580fc81fcf281b),
    C(fc0e30d682e87289), C(f44b784248d6107b), C(df25119527fdf209), C(cc265612588171a8)
  },
  {
    C(7ea73b6b74c8cd0b), C(e07188dd9b5bf3ca), C(6ef62ff2dd008ed4), C(acd94b3038342152), C(1b0ed99c9b7ba297), C(b794a93f4c895939), C(97a60cd93021206d),
    C(acd94b3038342152), C(1b0ed99c9b7ba297), C(b794a93f4c895939), C(97a60cd93021206d),
    C(9e0c0e6da5001b07), C(5f5b817de5d2a391), C(35b8a8702acdd533), C(3bbcfef344f455)
  },
  {
    C(e42ffdf6278bb21), C(59df3e5ca582ff9d), C(f3108785599dbde9), C(f78e8a2d4aba6a1d), C(700473fb0d8380fc), C(d0a0d68061ac74b2), C(11650612fa426e5a),
    C(f78e8a2d4aba6a1d), C(700473fb0d8380fc), C(d0a0d68061ac74b2), C(11650612fa426e5a),
    C(e39ceb5b2955710c), C(f559ff201f8cebaa), C(1fbc182809e829a0), C(295c7fc82fa6fb5b)
  },
  {
    C(9ad37fcd49fe4aa0), C(76d40da71930f708), C(bea08b630f731623), C(797292108901a81f), C(3b94127b18fae49c), C(688247179f144f1b), C(48a507a1625d13d7),
    C(797292108901a81f), C(3b94127b18fae49c), C(688247179f144f1b), C(48a507a1625d13d7),
    C(452322aaad817005), C(51d730d973e13d44), C(c883eb30176652ea), C(8d338fd678b2404d)
  },
  {
    C(27b7ff391136696e), C(60db94a18593438c), C(b5e46d79c4dafbad), C(ad56fd25a6f15289), C(68a0ec7c0179df80), C(a0aacfc36620957), C(87a0762a09e2e1c1),
    C(ad56fd25a6f15289), C(68a0ec7c0179df80), C(a0aacfc36620957), C(87a0762a09e2e1c1),
    C(d50ace99460f0be3), C(7f1fe5653ae0d999), C(3870899d9d6c22c), C(df5f952dd90d5a09)
  },
  {
    C(76bd077e42692ddf), C(c14b60958c2c7a85), C(fd9f3b0b3b1e2738), C(273d2c51a8e65e71), C(ac531423f670bf34), C(7f40c6bfb8c5758a), C(5fde65b433a10b02),
    C(273d2c51a8e65e71), C(ac531423f670bf34), C(7f40c6bfb8c5758a), C(5fde65b433a10b02),
    C(dbda6c4252b0a75c), C(5d4cfd8f937b23d9), C(3895f478e1c29c9d), C(e3e7c1fd1199aec6)
  },
  {
    C(81c672225442e053), C(927c3f6c8964050e), C(cb59f8f2bb36fac5), C(298f3583326fd942), C(b85602a9a2e2f97c), C(65c849bfa3191459), C(bf21329dfb496c0d),
    C(298f3583326fd942), C(b85602a9a2e2f97c), C(65c849bfa3191459), C(bf21329dfb496c0d),
    C(ea7b7b44c596aa18), C(c18bfb6e9a36d59c), C(1b55f03e8a38cc0a), C(b6a94cd47bbf847f)
  },
  {
    C(37b9e308747448ca), C(513f39f5545b1bd), C(145b32114ca00f9c), C(cce24b9910eb0489), C(af4ac64668ac57d9), C(ea0e44c13a9a5d5e), C(b224fb0c680455f4),
    C(cce24b9910eb0489), C(af4ac64668ac57d9), C(ea0e44c13a9a5d5e), C(b224fb0c680455f4),
    C(a7714bbba8699be7), C(fecad6e0e0092204), C(c1ce8bd5ac247eb4), C(3993aef5c07cdca2)
  },
  {
    C(dab71695950a51d4), C(9e98e4dfa07566fe), C(fab3587513b84ec0), C(2409f60f0854f305), C(b17f6e6c8ff1894c), C(62fa048551dc7ad6), C(d99f4fe2799bad72),
    C(2409f60f0854f305), C(b17f6e6c8ff1894c), C(62fa048551dc7ad6), C(d99f4fe2799bad72),
    C(4a38e7f2f4a669d3), C(53173510ca91f0e3), C(cc9096c0df860b0), C(52ed637026a4a0d5)
  },
  {
    C(28630288285c747b), C(a165a5bf51aaec95), C(927d211f27370016), C(727c782893d30c22), C(742706852989c247), C(c546494c3bb5e7e2), C(1fb2a5d1570f5dc0),
    C(727c782893d30c22), C(742706852989c247), C(c546494c3bb5e7e2), C(1fb2a5d1570f5dc0),
    C(71e498804df91b76), C(4a6a5aa6f7e5621), C(871a63730d13a544), C(63f77c8f371cc2f8)
  },
  {
    C(4b591ad5160b6c1b), C(e8f85ddd5a1143f7), C(377e18171476d64), C(829481773cce2cb1), C(c9d9fb4e25e4d243), C(c1fff894f0cf713b), C(69edd73ec20984b0),
    C(829481773cce2cb1), C(c9d9fb4e25e4d243), C(c1fff894f0cf713b), C(69edd73ec20984b0),
    C(7fb1132262925f4a), C(a292e214fe56794f), C(915bfee68e16f46f), C(98bcc857bb6d31e7)
  },
  {
    C(7e02f7a5a97dd3df), C(9724a88ac8c30809), C(d8dee12589eeaf36), C(c61f8fa31ad1885b), C(3e3744e04485ff9a), C(939335b37f34c7a2), C(faa5de308dbbbc39),
    C(c61f8fa31ad1885b), C(3e3744e04485ff9a), C(939335b37f34c7a2), C(faa5de308dbbbc39),
    C(f5996b1be7837a75), C(4fcb12d267f5af4f), C(39be67b8cd132169), C(5c39e3819198b8a1)
  },
  {
    C(ff66660873521fb2), C(d82841f7e714ce03), C(c830d273f005e378), C(66990c8c54782228), C(4f28bea83dda97c), C(6a24c64698688de0), C(69721141111da99b),
    C(66990c8c54782228), C(4f28bea83dda97c), C(6a24c64698688de0), C(69721141111da99b),
    C(d5c771fade83931b), C(8094ed75e6feb396), C(7a79d4de8efd1a2c), C(5f9e50167693e363)
  },
  {
    C(ef3c4dd60fa37412), C(e8d2898c86d11327), C(8c883d860aafacfe), C(a4ace72ba19d6de5), C(4cae26627dfc5511), C(38e496de9f677b05), C(558770996e1906d6),
    C(a4ace72ba19d6de5), C(4cae26627dfc5511), C(38e496de9f677b05), C(558770996e1906d6),
    C(40df30e332ceca69), C(8f106cbd94166c42), C(332b6ab4f4c1014e), C(7c0bc3092ad850e5)
  },
  {
    C(a7b07bcb1a1333ba), C(9d007956720914c3), C(4751f60ef2b15545), C(77ac4dcee10c9023), C(e90235108fa20e56), C(1d3ea38535215800), C(5ed1ccfff26bc64),
    C(77ac4dcee10c9023), C(e90235108fa20e56), C(1d3ea38535215800), C(5ed1ccfff26bc64),
    C(789a1c352bf5c61e), C(860a119056da8252), C(a6c268a238699086), C(4d70f5cccf4ef2eb)
  },
  {
    C(89858fc94ee25469), C(f72193b78aeaa896), C(7dba382760727c27), C(846b72f372f1685a), C(f708db2fead5433c), C(c04e121770ee5dc), C(4619793b67d0daa4),
    C(846b72f372f1685a), C(f708db2fead5433c), C(c04e121770ee5dc), C(4619793b67d0daa4),
    C(79f80506f152285f), C(5300074926fccd56), C(7fbbff6cc418fce6), C(b908f77c676b32e4)
  },
  {
    C(e6344d83aafdca2e), C(6e147816e6ebf87), C(8508c38680732caf), C(f4ce36d3a375c981), C(9d67e5572f8d7bf4), C(900d63d9ec79e477), C(5251c85ab52839a3),
    C(f4ce36d3a375c981), C(9d67e5572f8d7bf4), C(900d63d9ec79e477), C(5251c85ab52839a3),
    C(92ec4b3952e38027), C(40b2dc421a518cbf), C(661ea97b2331a070), C(8d428a4a9485179b)
  },
  {
    C(3ddbb400198d3d4d), C(fe73de3ada21af5c), C(cd7df833dacd8da3), C(162be779eea87bf8), C(7d62d36edf759e6d), C(dc20f528362e37b2), C(1a902edfe4a5824e),
    C(162be779eea87bf8), C(7d62d36edf759e6d), C(dc20f528362e37b2), C(1a902edfe4a5824e),
    C(e6a258d30fa817ba), C(c5d73adf6fb196fd), C(475b7a6286a207fb), C(d35f96363e8eba95)
  },
  {
    C(79d4c20cf83a7732), C(651ea0a6ab059bcd), C(94631144f363cdef), C(894a0ee0c1f87a22), C(4e682573f8b38f25), C(89803fc082816289), C(71613963a02d90e1),
    C(894a0ee0c1f87a22), C(4e682573f8b38f25), C(89803fc082816289), C(71613963a02d90e1),
    C(4c6cc0e5a737c910), C(a3765b5da16bccd9), C(8bf483c4d735ec96), C(7fd7c8ba1934afec)
  },
  {
    C(5aaf0d7b669173b5), C(19661ca108694547), C(5d03d681639d71fe), C(7c422f4a12fd1a66), C(aa561203e7413665), C(e99d8d202a04d573), C(6090357ec6f1f1),
    C(7c422f4a12fd1a66), C(aa561203e7413665), C(e99d8d202a04d573), C(6090357ec6f1f1),
    C(dbfe89f01f0162e), C(49aa89da4f1e389b), C(7119a6f4514efb22), C(56593f6b4e7318d9)
  },
  {
    C(35d6cc883840170c), C(444694c4f8928732), C(98500f14b8741c6), C(5021ac9480077dd), C(44c2ebc11cfb9837), C(e5d310c4b5c1d9fd), C(a577102c33ac773c),
    C(5021ac9480077dd), C(44c2ebc11cfb9837), C(e5d310c4b5c1d9fd), C(a577102c33ac773c),
    C(a00d2efd2effa3cf), C(c2c33ffcda749df6), C(d172099d3b6f2986), C(f308fe33fcd23338)
  },
  {
    C(b07eead7a57ff2fe), C(c1ffe295ca7dbf47), C(ef137b125cfa8851), C(8f8eec5cde7a490a), C(79916d20a405760b), C(3c30188c6d38c43c), C(b17e3c3ff7685e8d),
    C(8f8eec5cde7a490a), C(79916d20a405760b), C(3c30188c6d38c43c), C(b17e3c3ff7685e8d),
    C(ac8aa3cd0790c4c9), C(78ca60d8bf10f670), C(26f522be4fbc1184), C(55bc7688083326d4)
  },
  {
    C(20fba36c76380b18), C(95c39353c2a3477d), C(4f362902cf9117ad), C(89816ec851e3f405), C(65258396f932858d), C(b7dcaf3cc57a0017), C(b368f482afc90506),
    C(89816ec851e3f405), C(65258396f932858d), C(b7dcaf3cc57a0017), C(b368f482afc90506),
    C(88f08c74465015f1), C(94ebaf209d59099d), C(c1b7ff7304b0a87), C(56bf8235257d4435)
  },
  {
    C(c7e9e0c45afeab41), C(999d95f41d9ee841), C(55ef15ac11ea010), C(cc951b8eab5885d), C(956c702c88ac056b), C(de355f324a37e3c0), C(ed09057eb60bd463),
    C(cc951b8eab5885d), C(956c702c88ac056b), C(de355f324a37e3c0), C(ed09057eb60bd463),
    C(1f44b6d04a43d088), C(53631822a26ba96d), C(90305fc2d21f8d28), C(60693a9a6093351a)
  },
  {
    C(69a8e59c1577145d), C(cb04a6e309ebc626), C(9b3326a5b250e9b1), C(d805f665265fd867), C(82b2b019652c19c6), C(f0df7738353c82a6), C(6a9acf124383ca5f),
    C(d805f665265fd867), C(82b2b019652c19c6), C(f0df7738353c82a6), C(6a9acf124383ca5f),
    C(6838374508a7a99f), C(7b6719db8d3e40af), C(1a22666cf0dcb7cf), C(989a9cf7f46b434d)
  },
  {
    C(6638191337226509), C(42b55e08e4894870), C(a7696f5fbd51878e), C(433bbdd27481d85d), C(ee32136b5a47bbec), C(769a77f346d82f4e), C(38b91b1cb7e34be),
    C(433bbdd27481d85d), C(ee32136b5a47bbec), C(769a77f346d82f4e), C(38b91b1cb7e34be),
    C(cb10fd95c0e43875), C(ce9744efd6f11427), C(946b32bddada6a13), C(35d544690b99e3b6)
  },
  {
    C(c44e8c33ff6c5e13), C(1f128a22aab3007f), C(6a8b41bf04cd593), C(1b9b0deaf126522a), C(cc51d382baedc2eb), C(8df8831bb2e75daa), C(de4e7a4b5de99588),
    C(1b9b0deaf126522a), C(cc51d382baedc2eb), C(8df8831bb2e75daa), C(de4e7a4b5de99588),
    C(55a2707103a9f968), C(e0063f4e1649702d), C(7e82f5b440e74043), C(649b44a27f00219d)
  },
  {
    C(68125009158bded7), C(563a9a62753fc088), C(b97a9873a352cf6a), C(237d1de15ae56127), C(b96445f758ba57d), C(b842628a9f9938eb), C(70313d232dc2cd0d),
    C(237d1de15ae56127), C(b96445f758ba57d), C(b842628a9f9938eb), C(70313d232dc2cd0d),
    C(8bfe1f78cb40ad5b), C(a5bde811d49f56e1), C(1acd0cf913ded507), C(820b3049fa5e6786)
  },
  {
    C(e0dd644db35a62d6), C(292889772752ab42), C(b80433749dbb8793), C(7032fe67035f95db), C(d8076d1fda17eb8d), C(115ca1775560f946), C(92da1e16f396bf61),
    C(7032fe67035f95db), C(d8076d1fda17eb8d), C(115ca1775560f946), C(92da1e16f396bf61),
    C(17c8bc7f6d23a639), C(fb28a2afa4d562a9), C(6c59c95fa2450d5f), C(fe0d41d5ebfbce2a)
  },
  {
    C(21ce9eab220aaf87), C(27d20caec922d708), C(610c51f976cb1d30), C(6052f97a1e02d2ba), C(836eea7ce63dea17), C(e1f8efb81b443b45), C(ddbdbbe717570246),
    C(6052f97a1e02d2ba), C(836eea7ce63dea17), C(e1f8efb81b443b45), C(ddbdbbe717570246),
    C(69551045b0e56f60), C(625a435960ba7466), C(9cdb004e8b11405c), C(d6284db99a3b16af)
  },
  {
    C(83b54046fdca7c1e), C(e3709e9153c01626), C(f306b5edc2682490), C(88f14b0b554fba02), C(a0ec13fac0a24d0), C(f468ebbc03b05f47), C(a9cc417c8dad17f0),
    C(88f14b0b554fba02), C(a0ec13fac0a24d0), C(f468ebbc03b05f47), C(a9cc417c8dad17f0),
    C(4c1ababa96d42275), C(c112895a2b751f17), C(5dd7d9fa55927aa9), C(ca09db548d91cd46)
  },
  {
    C(dd3b2ce7dabb22fb), C(64888c62a5cb46ee), C(f004e8b4b2a97362), C(31831cf3efc20c84), C(901ba53808e677ae), C(4b36895c097d0683), C(7d93ad993f9179aa),
    C(31831cf3efc20c84), C(901ba53808e677ae), C(4b36895c097d0683), C(7d93ad993f9179aa),
    C(a4c5ea29ae78ba6b), C(9cf637af6d607193), C(5731bd261d5b3adc), C(d59a9e4f796984f3)
  },
  {
    C(9ee08fc7a86b0ea6), C(5c8d17dff5768e66), C(18859672bafd1661), C(d3815c5f595e513e), C(44b3bdbdc0fe061f), C(f5f43b2a73ad2df5), C(7c0e6434c8d7553c),
    C(d3815c5f595e513e), C(44b3bdbdc0fe061f), C(f5f43b2a73ad2df5), C(7c0e6434c8d7553c),
    C(8c05859060821002), C(73629a0d275008ce), C(860c012879e6f00f), C(d48735a120d2c37c)
  },
  {
    C(4e2a10f1c409dfa5), C(6e684591f5da86bd), C(ff8c9305d447cadb), C(c43ae49df25b1c86), C(d4f42115cee1ac8), C(a0e6a714471b975c), C(a40089dec5fe07b0),
    C(c43ae49df25b1c86), C(d4f42115cee1ac8), C(a0e6a714471b975c), C(a40089dec5fe07b0),
    C(18c3d8f967915e10), C(739c747dbe05adfb), C(4b0397b596e16230), C(3c57d7e1de9e58d1)
  },
  {
    C(bdf3383d7216ee3c), C(eed3a37e4784d324), C(247cff656d081ba0), C(76059e4cb25d4700), C(e0af815fe1fa70ed), C(5a6ccb4f36c5b3df), C(391a274cd5f5182d),
    C(76059e4cb25d4700), C(e0af815fe1fa70ed), C(5a6ccb4f36c5b3df), C(391a274cd5f5182d),
    C(ff1579baa6a2b511), C(c385fc5062e8a728), C(e940749739a37c78), C(a093523a5b5edee5)
  },
  {
    C(a22e8f6681f0267d), C(61e79bc120729914), C(86ec13c84c1600d3), C(1614811d59dcab44), C(d1ddcca9a2675c33), C(f3c551d5fa617763), C(5c78d4181402e98c),
    C(1614811d59dcab44), C(d1ddcca9a2675c33), C(f3c551d5fa617763), C(5c78d4181402e98c),
    C(b43b4a9caa6f5d4c), C(f112829bca2df8f3), C(87e5c85db80d06c3), C(8eb4bac85453409)
  },
  {
    C(6997121cae0ce362), C(ba3594cbcc299a07), C(7e4b71c7de25a5e4), C(16ad89e66db557ba), C(a43c401140ffc77d), C(3780a8b3fd91e68), C(48190678248a06b5),
    C(16ad89e66db557ba), C(a43c401140ffc77d), C(3780a8b3fd91e68), C(48190678248a06b5),
    C(d10deb97b651ad42), C(3a69f3f29046a24f), C(f7179735f2c6dab4), C(ac82965ad3b67a02)
  },
  {
    C(9bfc2c3e050a3c27), C(dc434110e1059ff3), C(5426055da178decd), C(cb44d00207e16f99), C(9d9e99afedc8107f), C(56907c4fb7b3bc01), C(bcff1472bb01f85a),
    C(cb44d00207e16f99), C(9d9e99afedc8107f), C(56907c4fb7b3bc01), C(bcff1472bb01f85a),
    C(516f800f74ad0985), C(f93193ade9614da4), C(9f4a7845355b75b7), C(423c17045824dea5)
  },
  {
    C(a3f37e415aedf14), C(8d21c92bfa0dc545), C(a2715ebb07deaf80), C(98ce1ff2b3f99f0f), C(162acfd3b47c20bf), C(62b9a25fd39dc6c0), C(c165c3c95c878dfe),
    C(98ce1ff2b3f99f0f), C(162acfd3b47c20bf), C(62b9a25fd39dc6c0), C(c165c3c95c878dfe),
    C(2b9a7e1f055bd27c), C(e91c8099cafaa75d), C(37e38d64ef0263b), C(a46e89f47a1a70d5)
  },
  {
    C(cef3c748045e7618), C(41dd44faef4ca301), C(6add718a88f383c6), C(1197eca317e70a93), C(61f9497e6cc4a33), C(22e7178d1e57af73), C(5df95da0ff1c6435),
    C(1197eca317e70a93), C(61f9497e6cc4a33), C(22e7178d1e57af73), C(5df95da0ff1c6435),
    C(934327643705e56c), C(11eb0eec553137c9), C(1e6b9b57ac5283ec), C(6934785db184b2e4)
  },
  {
    C(fe2b841766a4d212), C(42cf817e58fe998c), C(29f7f493ba9cbe6c), C(2a9231d98b441827), C(fca55e769df78f6c), C(da87ea680eb14df4), C(e0b77394b0fd2bcc),
    C(2a9231d98b441827), C(fca55e769df78f6c), C(da87ea680eb14df4), C(e0b77394b0fd2bcc),
    C(f36a2a3c73ab371a), C(d52659d36d93b71), C(3d3b7d2d2fafbb14), C(b4b7b317d9266711)
  },
  {
    C(d6131e688593a181), C(5b658b282688ccd3), C(b9f7c066beed1204), C(e9dd79bad89f6b19), C(b420092bae6aaf41), C(515f9bbd06069d77), C(80664957a02cbc29),
    C(e9dd79bad89f6b19), C(b420092bae6aaf41), C(515f9bbd06069d77), C(80664957a02cbc29),
    C(f9dc7a744a56d9b3), C(7eb2bdcd6667f383), C(c5914296fbdaf9d1), C(af0d5a8fec374fc4)
  },
  {
    C(91288884ebfcf145), C(3dffd892d36403af), C(7c4789db82755080), C(634acbe037edec27), C(878a97fab822d804), C(fcb042af908f0577), C(4cbafc318bb90a2e),
    C(634acbe037edec27), C(878a97fab822d804), C(fcb042af908f0577), C(4cbafc318bb90a2e),
    C(68a96d589d5e5654), C(a752cb250bca1bc0), C(8f228f406024aa7e), C(fc5408cf22a080b5)
  },
  {
    C(754c7e98ae3495ea), C(2030124a22512c19), C(ec241579c626c39d), C(e682b5c87fa8e41b), C(6cfa4baff26337ac), C(4d66358112f09b2a), C(58889d3f50ffa99c),
    C(33fc6ffd1ffb8676), C(36db7617b765f6e2), C(8df41c03c514a9dc), C(6707cc39a809bb74),
    C(3f27d7bb79e31984), C(a39dc6ac6cb0b0a8), C(33fc6ffd1ffb8676), C(36db7617b765f6e2)
  },
};


// Initialize data to pseudorandom values.
#define SETUP()                                         \
  do {                                                  \
    uint64 _a = 9;                                      \
    uint64 _b = 777;                                    \
    uint8 _u;                                           \
    for (int _i = 0; _i < kDataSize; _i++) {            \
      _a = (_a ^ (_a >> 41)) * k0 + _b;                 \
      _b = (_b ^ (_b >> 41)) * k0 + (uint64)_i;         \
      _u = (uint8)((_b >> 37) & 0xff);                  \
      memcpy(data + _i, &_u, 1);  /* uint8 -> char */   \
    }                                                   \
  } while (0)

#define CHECK(_e, _a, __len)                                            \
  do {                                                                  \
    uint64 __e;                                                         \
    uint64 __a;                                                         \
    __e = (uint64)(_e);                                                 \
    __a = (uint64)(_a);                                                 \
    if (__e != __a) {                                                   \
      fprintf(stderr, "error: %s, len = %u, expected %lu (0x%lx), actual %lu (0x%lx)\n", \
              (# _a),                                                   \
              (unsigned)(__len),                                        \
              (unsigned long)__e, (unsigned long)__e,                   \
              (unsigned long)__a, (unsigned long)__a);                  \
      ++errors;                                                         \
    }                                                                   \
  } while (0)

#define TEST(_expected, _offset, _len)                                  \
  do {                                                                  \
    const uint128 _u = CityHash128(data + (_offset), (_len));           \
    const uint128 _v = CityHash128WithSeed(data + (_offset), (_len), kSeed128.first, kSeed128.second); \
    CHECK((_expected)[0], CityHash64(data + (_offset), (_len)), (_len)); \
    CHECK((_expected)[1], CityHash64WithSeed(data + (_offset), (_len), kSeed0), (_len)); \
    CHECK((_expected)[2], CityHash64WithSeeds(data + (_offset), (_len), kSeed0, kSeed1), (_len)); \
    CHECK((_expected)[3], Uint128Low64(_u), (_len));                    \
    CHECK((_expected)[4], Uint128High64(_u), (_len));                   \
    CHECK((_expected)[5], Uint128Low64(_v), (_len));                    \
    CHECK((_expected)[6], Uint128High64(_v), (_len));                   \
    TEST_SSE42((_expected), (_offset), (_len));                         \
  } while (0)

#ifdef __SSE4_2__
#define TEST_SSE42(_expected, _offset, _len)                            \
  do {                                                                  \
    const uint128 _y = CityHashCrc128(data + (_offset), (_len));        \
    const uint128 _z = CityHashCrc128WithSeed(data + (_offset), (_len), kSeed128); \
    uint64 _crc256_results[4];                                          \
    CityHashCrc256(data + (_offset), (_len), _crc256_results);          \
    CHECK((_expected)[7], Uint128Low64(_y), (_len));                    \
    CHECK((_expected)[8], Uint128High64(_y), (_len));                   \
    CHECK((_expected)[9], Uint128Low64(_z), (_len));                    \
    CHECK((_expected)[10], Uint128High64(_z), (_len));                  \
    for (int __i = 0; __i < 4; __i++) {                                 \
      CHECK((_expected)[11 + __i], _crc256_results[__i], (_len));       \
    }                                                                   \
  } while (0)
#else /* __SSE4_2__ */
#define TEST_SSE42(_expected, _offset, _len)    \
  do {                                          \
    /* NOP */                                   \
  } while (0)
#endif /* __SSE4_2__ */

#define TEST_RUN_CITYHASH()                             \
  do {                                                  \
    int _i;                                             \
    for (_i = 0 ; _i < kTestSize - 1; _i++) {           \
      TEST(testdata[_i], _i * _i, (size_t)_i);          \
    }                                                   \
    TEST(testdata[kTestSize - 1], 0, (size_t)kDataSize);        \
  } while (0)

#endif /* __SRC_DATAPLANE_OFPROTO_TEST_CITYHASH_TEST_IMPORTED_H__ */

