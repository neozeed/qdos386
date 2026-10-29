/* ROM section tb_f0 */
.balign 2,0
/* QL: tb_f0 */
tb_f0:
/* QL:  dc.b $1f,(M192_tb_f0x-tb_f0-11)/9 first character and total number less one */
M192_dc_anchor_72:
.byte (0x1f)&255
.byte ((((M192_tb_f0x-tb_f0)-11)/9))&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_76:
.byte (M192_oooo.)&255
/* QL:  dc.b O.... */
M192_dc_anchor_79:
.byte (M192_o....)&255
/* QL:  dc.b OOO.. */
M192_dc_anchor_82:
.byte (M192_ooo..)&255
/* QL:  dc.b O.... */
M192_dc_anchor_85:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_88:
.byte (M192_o....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_91:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_94:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_97:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_100:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_103:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_106:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_109:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_112:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_115:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_118:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_121:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_124:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_127:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_130:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_133:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_136:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_139:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_142:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_145:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_148:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_151:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_154:
.byte (.....)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_157:
.byte (.o.o.)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_160:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_163:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_166:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_169:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_172:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_175:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_178:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_181:
.byte (.....)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_184:
.byte (.o.o.)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_187:
.byte (.o.o.)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_190:
.byte (M192_ooooo)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_193:
.byte (.o.o.)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_196:
.byte (M192_ooooo)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_199:
.byte (.o.o.)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_202:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_205:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_208:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_211:
.byte (.ooo.)&255
/* QL:  dc.b O.O.. */
M192_dc_anchor_214:
.byte (M192_o.o..)&255
/* QL:  dc.b O.O.. */
M192_dc_anchor_217:
.byte (M192_o.o..)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_220:
.byte (.ooo.)&255
/* QL:  dc.b ..O.O */
M192_dc_anchor_223:
.byte (..o.o)&255
/* QL:  dc.b ..O.O */
M192_dc_anchor_226:
.byte (..o.o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_229:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_232:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_235:
.byte (.....)&255
/* QL:  dc.b OO..O */
M192_dc_anchor_238:
.byte (M192_oo..o)&255
/* QL:  dc.b OO..O */
M192_dc_anchor_241:
.byte (M192_oo..o)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_244:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_247:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_250:
.byte (.o...)&255
/* QL:  dc.b O..OO */
M192_dc_anchor_253:
.byte (M192_o..oo)&255
/* QL:  dc.b O..OO */
M192_dc_anchor_256:
.byte (M192_o..oo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_259:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_262:
.byte (.....)&255
/* QL:  dc.b .O... */
M192_dc_anchor_265:
.byte (.o...)&255
/* QL:  dc.b O.O.. */
M192_dc_anchor_268:
.byte (M192_o.o..)&255
/* QL:  dc.b O.O.. */
M192_dc_anchor_271:
.byte (M192_o.o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_274:
.byte (.o...)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_277:
.byte (M192_o.o.o)&255
/* QL:  dc.b O..O. */
M192_dc_anchor_280:
.byte (M192_o..o.)&255
/* QL:  dc.b .OO.O */
M192_dc_anchor_283:
.byte (.oo.o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_286:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_289:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_292:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_295:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_298:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_301:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_304:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_307:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_310:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_313:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_316:
.byte (.....)&255
/* QL:  dc.b ....O */
M192_dc_anchor_319:
.byte (....o)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_322:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_325:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_328:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_331:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_334:
.byte (...o.)&255
/* QL:  dc.b ....O */
M192_dc_anchor_337:
.byte (....o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_340:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_343:
.byte (.....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_346:
.byte (M192_o....)&255
/* QL:  dc.b .O... */
M192_dc_anchor_349:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_352:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_355:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_358:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_361:
.byte (.o...)&255
/* QL:  dc.b O.... */
M192_dc_anchor_364:
.byte (M192_o....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_367:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_370:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_373:
.byte (..o..)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_376:
.byte (M192_o.o.o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_379:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_382:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_385:
.byte (.ooo.)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_388:
.byte (M192_o.o.o)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_391:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_394:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_397:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_400:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_403:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_406:
.byte (..o..)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_409:
.byte (M192_ooooo)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_412:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_415:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_418:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_421:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_424:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_427:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_430:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_433:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_436:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_439:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_442:
.byte (..oo.)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_445:
.byte (..oo.)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_448:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_451:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_454:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_457:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_460:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_463:
.byte (M192_ooooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_466:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_469:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_472:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_475:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_478:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_481:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_484:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_487:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_490:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_493:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_496:
.byte (..oo.)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_499:
.byte (..oo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_502:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_505:
.byte (.....)&255
/* QL:  dc.b ....O */
M192_dc_anchor_508:
.byte (....o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_511:
.byte (....o)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_514:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_517:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_520:
.byte (.o...)&255
/* QL:  dc.b O.... */
M192_dc_anchor_523:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_526:
.byte (M192_o....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_529:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_532:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_535:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_538:
.byte (M192_o...o)&255
/* QL:  dc.b O..OO */
M192_dc_anchor_541:
.byte (M192_o..oo)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_544:
.byte (M192_o.o.o)&255
/* QL:  dc.b OO..O */
M192_dc_anchor_547:
.byte (M192_oo..o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_550:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_553:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_556:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_559:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_562:
.byte (..o..)&255
/* QL:  dc.b .OO.. */
M192_dc_anchor_565:
.byte (.oo..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_568:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_571:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_574:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_577:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_580:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_583:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_586:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_589:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_592:
.byte (M192_o...o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_595:
.byte (....o)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_598:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_601:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_604:
.byte (.o...)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_607:
.byte (M192_ooooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_610:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_613:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_616:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_619:
.byte (M192_o...o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_622:
.byte (....o)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_625:
.byte (..oo.)&255
/* QL:  dc.b ....O */
M192_dc_anchor_628:
.byte (....o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_631:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_634:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_637:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_640:
.byte (.....)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_643:
.byte (...o.)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_646:
.byte (..oo.)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_649:
.byte (.o.o.)&255
/* QL:  dc.b O..O. */
M192_dc_anchor_652:
.byte (M192_o..o.)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_655:
.byte (M192_ooooo)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_658:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_661:
.byte (...o.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_664:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_667:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_670:
.byte (M192_ooooo)&255
/* QL:  dc.b O.... */
M192_dc_anchor_673:
.byte (M192_o....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_676:
.byte (M192_oooo.)&255
/* QL:  dc.b ....O */
M192_dc_anchor_679:
.byte (....o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_682:
.byte (....o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_685:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_688:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_691:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_694:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_697:
.byte (..oo.)&255
/* QL:  dc.b .O... */
M192_dc_anchor_700:
.byte (.o...)&255
/* QL:  dc.b O.... */
M192_dc_anchor_703:
.byte (M192_o....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_706:
.byte (M192_oooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_709:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_712:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_715:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_718:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_721:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_724:
.byte (M192_ooooo)&255
/* QL:  dc.b ....O */
M192_dc_anchor_727:
.byte (....o)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_730:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_733:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_736:
.byte (.o...)&255
/* QL:  dc.b O.... */
M192_dc_anchor_739:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_742:
.byte (M192_o....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_745:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_748:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_751:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_754:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_757:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_760:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_763:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_766:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_769:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_772:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_775:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_778:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_781:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_784:
.byte (M192_o...o)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_787:
.byte (.oooo)&255
/* QL:  dc.b ....O */
M192_dc_anchor_790:
.byte (....o)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_793:
.byte (...o.)&255
/* QL:  dc.b .OO.. */
M192_dc_anchor_796:
.byte (.oo..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_799:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_802:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_805:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_808:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_811:
.byte (..oo.)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_814:
.byte (..oo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_817:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_820:
.byte (..oo.)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_823:
.byte (..oo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_826:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_829:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_832:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_835:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_838:
.byte (..oo.)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_841:
.byte (..oo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_844:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_847:
.byte (..oo.)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_850:
.byte (..oo.)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_853:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_856:
.byte (..o..)&255
/* QL:  dc.b ....O */
M192_dc_anchor_859:
.byte (....o)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_862:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_865:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_868:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_871:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_874:
.byte (...o.)&255
/* QL:  dc.b ....O */
M192_dc_anchor_877:
.byte (....o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_880:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_883:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_886:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_889:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_892:
.byte (M192_ooooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_895:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_898:
.byte (M192_ooooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_901:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_904:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_907:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_910:
.byte (.....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_913:
.byte (M192_o....)&255
/* QL:  dc.b .O... */
M192_dc_anchor_916:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_919:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_922:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_925:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_928:
.byte (.o...)&255
/* QL:  dc.b O.... */
M192_dc_anchor_931:
.byte (M192_o....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_934:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_937:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_940:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_943:
.byte (M192_o...o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_946:
.byte (....o)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_949:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_952:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_955:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_958:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_961:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_964:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_967:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_970:
.byte (M192_o...o)&255
/* QL:  dc.b O..OO */
M192_dc_anchor_973:
.byte (M192_o..oo)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_976:
.byte (M192_o.o.o)&255
/* QL:  dc.b O..OO */
M192_dc_anchor_979:
.byte (M192_o..oo)&255
/* QL:  dc.b O.... */
M192_dc_anchor_982:
.byte (M192_o....)&255
/* QL:  dc.b .OO.. */
M192_dc_anchor_985:
.byte (.oo..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_988:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_991:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_994:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_997:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1000:
.byte (M192_o...o)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1003:
.byte (M192_ooooo)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1006:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1009:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1012:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1015:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1018:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1021:
.byte (M192_oooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1024:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1027:
.byte (M192_o...o)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1030:
.byte (M192_oooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1033:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1036:
.byte (M192_o...o)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1039:
.byte (M192_oooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1042:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1045:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1048:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1051:
.byte (M192_o...o)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1054:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1057:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1060:
.byte (M192_o....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1063:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1066:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1069:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1072:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1075:
.byte (M192_oooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1078:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1081:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1084:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1087:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1090:
.byte (M192_o...o)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1093:
.byte (M192_oooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1096:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1099:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1102:
.byte (M192_ooooo)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1105:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1108:
.byte (M192_o....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1111:
.byte (M192_oooo.)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1114:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1117:
.byte (M192_o....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1120:
.byte (M192_ooooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1123:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1126:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1129:
.byte (M192_ooooo)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1132:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1135:
.byte (M192_o....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1138:
.byte (M192_oooo.)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1141:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1144:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1147:
.byte (M192_o....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1150:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1153:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1156:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1159:
.byte (M192_o...o)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1162:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1165:
.byte (M192_o....)&255
/* QL:  dc.b O..OO */
M192_dc_anchor_1168:
.byte (M192_o..oo)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1171:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1174:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1177:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1180:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1183:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1186:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1189:
.byte (M192_o...o)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1192:
.byte (M192_ooooo)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1195:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1198:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1201:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1204:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1207:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1210:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1213:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1216:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1219:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1222:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1225:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1228:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1231:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1234:
.byte (.....)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1237:
.byte (....o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1240:
.byte (....o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1243:
.byte (....o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1246:
.byte (....o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1249:
.byte (....o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1252:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1255:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1258:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1261:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1264:
.byte (M192_o...o)&255
/* QL:  dc.b O..O. */
M192_dc_anchor_1267:
.byte (M192_o..o.)&255
/* QL:  dc.b O.O.. */
M192_dc_anchor_1270:
.byte (M192_o.o..)&255
/* QL:  dc.b OO... */
M192_dc_anchor_1273:
.byte (M192_oo...)&255
/* QL:  dc.b O.O.. */
M192_dc_anchor_1276:
.byte (M192_o.o..)&255
/* QL:  dc.b O..O. */
M192_dc_anchor_1279:
.byte (M192_o..o.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1282:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1285:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1288:
.byte (.....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1291:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1294:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1297:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1300:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1303:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1306:
.byte (M192_o....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1309:
.byte (M192_ooooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1312:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1315:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1318:
.byte (M192_o...o)&255
/* QL:  dc.b OO.OO */
M192_dc_anchor_1321:
.byte (M192_oo.oo)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_1324:
.byte (M192_o.o.o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1327:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1330:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1333:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1336:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1339:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1342:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1345:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1348:
.byte (M192_o...o)&255
/* QL:  dc.b OO..O */
M192_dc_anchor_1351:
.byte (M192_oo..o)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_1354:
.byte (M192_o.o.o)&255
/* QL:  dc.b O..OO */
M192_dc_anchor_1357:
.byte (M192_o..oo)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1360:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1363:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1366:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1369:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1372:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1375:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1378:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1381:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1384:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1387:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1390:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1393:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1396:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1399:
.byte (M192_oooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1402:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1405:
.byte (M192_o...o)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1408:
.byte (M192_oooo.)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1411:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1414:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1417:
.byte (M192_o....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1420:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1423:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1426:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1429:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1432:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1435:
.byte (M192_o...o)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_1438:
.byte (M192_o.o.o)&255
/* QL:  dc.b O..O. */
M192_dc_anchor_1441:
.byte (M192_o..o.)&255
/* QL:  dc.b .OO.O */
M192_dc_anchor_1444:
.byte (.oo.o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1447:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1450:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1453:
.byte (M192_oooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1456:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1459:
.byte (M192_o...o)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1462:
.byte (M192_oooo.)&255
/* QL:  dc.b O.O.. */
M192_dc_anchor_1465:
.byte (M192_o.o..)&255
/* QL:  dc.b O..O. */
M192_dc_anchor_1468:
.byte (M192_o..o.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1471:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1474:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1477:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1480:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1483:
.byte (M192_o...o)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1486:
.byte (M192_o....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1489:
.byte (.ooo.)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1492:
.byte (....o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1495:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1498:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1501:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1504:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1507:
.byte (M192_ooooo)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1510:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1513:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1516:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1519:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1522:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1525:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1528:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1531:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1534:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1537:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1540:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1543:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1546:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1549:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1552:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1555:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1558:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1561:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1564:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1567:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1570:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1573:
.byte (M192_o...o)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_1576:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1579:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1582:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1585:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1588:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1591:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1594:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1597:
.byte (M192_o...o)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_1600:
.byte (M192_o.o.o)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_1603:
.byte (M192_o.o.o)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_1606:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1609:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1612:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1615:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1618:
.byte (M192_o...o)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_1621:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1624:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_1627:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1630:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1633:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1636:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1639:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1642:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1645:
.byte (M192_o...o)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_1648:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1651:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1654:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1657:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1660:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1663:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1666:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1669:
.byte (M192_ooooo)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1672:
.byte (....o)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_1675:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1678:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_1681:
.byte (.o...)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1684:
.byte (M192_o....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1687:
.byte (M192_ooooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1690:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1693:
.byte (.....)&255
/* QL:  dc.b ..OOO */
M192_dc_anchor_1696:
.byte (..ooo)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1699:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1702:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1705:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1708:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1711:
.byte (..o..)&255
/* QL:  dc.b ..OOO */
M192_dc_anchor_1714:
.byte (..ooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1717:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1720:
.byte (.....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1723:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1726:
.byte (M192_o....)&255
/* QL:  dc.b .O... */
M192_dc_anchor_1729:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1732:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_1735:
.byte (...o.)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1738:
.byte (....o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1741:
.byte (....o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1744:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1747:
.byte (.....)&255
/* QL:  dc.b OOO.. */
M192_dc_anchor_1750:
.byte (M192_ooo..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1753:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1756:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1759:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1762:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1765:
.byte (..o..)&255
/* QL:  dc.b OOO.. */
M192_dc_anchor_1768:
.byte (M192_ooo..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1771:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1774:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_1777:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_1780:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1783:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1786:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1789:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1792:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1795:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1798:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1801:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1804:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1807:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1810:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1813:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1816:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1819:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1822:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1825:
.byte (M192_ooooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1828:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_1831:
.byte (..oo.)&255
/* QL:  dc.b .O..O */
M192_dc_anchor_1834:
.byte (.o..o)&255
/* QL:  dc.b .O... */
M192_dc_anchor_1837:
.byte (.o...)&255
/* QL:  dc.b OOO.. */
M192_dc_anchor_1840:
.byte (M192_ooo..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_1843:
.byte (.o...)&255
/* QL:  dc.b .O... */
M192_dc_anchor_1846:
.byte (.o...)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1849:
.byte (M192_ooooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1852:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1855:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1858:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1861:
.byte (.....)&255
/* QL:  dc.b .OO.O */
M192_dc_anchor_1864:
.byte (.oo.o)&255
/* QL:  dc.b O..OO */
M192_dc_anchor_1867:
.byte (M192_o..oo)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1870:
.byte (M192_o...o)&255
/* QL:  dc.b O..OO */
M192_dc_anchor_1873:
.byte (M192_o..oo)&255
/* QL:  dc.b .OO.O */
M192_dc_anchor_1876:
.byte (.oo.o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1879:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1882:
.byte (.....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1885:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1888:
.byte (M192_o....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1891:
.byte (M192_oooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1894:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1897:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1900:
.byte (M192_o...o)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_1903:
.byte (M192_oooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1906:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1909:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1912:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1915:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_1918:
.byte (.oooo)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1921:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1924:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1927:
.byte (M192_o....)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_1930:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1933:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1936:
.byte (.....)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1939:
.byte (....o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_1942:
.byte (....o)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_1945:
.byte (.oooo)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1948:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1951:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1954:
.byte (M192_o...o)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_1957:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1960:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1963:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1966:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1969:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_1972:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_1975:
.byte (M192_o...o)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_1978:
.byte (M192_ooooo)&255
/* QL:  dc.b O.... */
M192_dc_anchor_1981:
.byte (M192_o....)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_1984:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1987:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_1990:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M192_dc_anchor_1993:
.byte (..oo.)&255
/* QL:  dc.b .O..O */
M192_dc_anchor_1996:
.byte (.o..o)&255
/* QL:  dc.b .O... */
M192_dc_anchor_1999:
.byte (.o...)&255
/* QL:  dc.b OOO.. */
M192_dc_anchor_2002:
.byte (M192_ooo..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_2005:
.byte (.o...)&255
/* QL:  dc.b .O... */
M192_dc_anchor_2008:
.byte (.o...)&255
/* QL:  dc.b .O... */
M192_dc_anchor_2011:
.byte (.o...)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2014:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2017:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2020:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2023:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2026:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2029:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2032:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2035:
.byte (M192_o...o)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_2038:
.byte (.oooo)&255
/* QL:  dc.b ....O */
M192_dc_anchor_2041:
.byte (....o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2044:
.byte (.ooo.)&255
/* QL:  dc.b O.... */
M192_dc_anchor_2047:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_2050:
.byte (M192_o....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_2053:
.byte (M192_oooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2056:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2059:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2062:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2065:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2068:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2071:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2074:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2077:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2080:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2083:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2086:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2089:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_2092:
.byte (...o.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2095:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2098:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2101:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2104:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2107:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2110:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2113:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2116:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2119:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2122:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_2125:
.byte (.o...)&255
/* QL:  dc.b O.... */
M192_dc_anchor_2128:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_2131:
.byte (M192_o....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2134:
.byte (M192_o...o)&255
/* QL:  dc.b O..O. */
M192_dc_anchor_2137:
.byte (M192_o..o.)&255
/* QL:  dc.b OOO.. */
M192_dc_anchor_2140:
.byte (M192_ooo..)&255
/* QL:  dc.b O..O. */
M192_dc_anchor_2143:
.byte (M192_o..o.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2146:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2149:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2152:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2155:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2158:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2161:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2164:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2167:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2170:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_2173:
.byte (...o.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2176:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2179:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2182:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2185:
.byte (.....)&255
/* QL:  dc.b OO.O. */
M192_dc_anchor_2188:
.byte (M192_oo.o.)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_2191:
.byte (M192_o.o.o)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_2194:
.byte (M192_o.o.o)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_2197:
.byte (M192_o.o.o)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_2200:
.byte (M192_o.o.o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2203:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2206:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2209:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2212:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_2215:
.byte (M192_oooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2218:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2221:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2224:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2227:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2230:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2233:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2236:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2239:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2242:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2245:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2248:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2251:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2254:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2257:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2260:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2263:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2266:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_2269:
.byte (M192_oooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2272:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2275:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2278:
.byte (M192_o...o)&255
/* QL:  dc.b OOOO. */
M192_dc_anchor_2281:
.byte (M192_oooo.)&255
/* QL:  dc.b O.... */
M192_dc_anchor_2284:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_2287:
.byte (M192_o....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2290:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2293:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_2296:
.byte (.oooo)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2299:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2302:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2305:
.byte (M192_o...o)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_2308:
.byte (.oooo)&255
/* QL:  dc.b ....O */
M192_dc_anchor_2311:
.byte (....o)&255
/* QL:  dc.b ....O */
M192_dc_anchor_2314:
.byte (....o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2317:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2320:
.byte (.....)&255
/* QL:  dc.b O.OO. */
M192_dc_anchor_2323:
.byte (M192_o.oo.)&255
/* QL:  dc.b OO..O */
M192_dc_anchor_2326:
.byte (M192_oo..o)&255
/* QL:  dc.b O.... */
M192_dc_anchor_2329:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_2332:
.byte (M192_o....)&255
/* QL:  dc.b O.... */
M192_dc_anchor_2335:
.byte (M192_o....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2338:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2341:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2344:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2347:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2350:
.byte (.ooo.)&255
/* QL:  dc.b O.... */
M192_dc_anchor_2353:
.byte (M192_o....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2356:
.byte (.ooo.)&255
/* QL:  dc.b ....O */
M192_dc_anchor_2359:
.byte (....o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2362:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2365:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2368:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2371:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2374:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2377:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2380:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2383:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2386:
.byte (..o..)&255
/* QL:  dc.b ...OO */
M192_dc_anchor_2389:
.byte (...oo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2392:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2395:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2398:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2401:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2404:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2407:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2410:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2413:
.byte (M192_o...o)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_2416:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2419:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2422:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2425:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2428:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2431:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2434:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2437:
.byte (M192_o...o)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_2440:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2443:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2446:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2449:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2452:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2455:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2458:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2461:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2464:
.byte (M192_o...o)&255
/* QL:  dc.b O.O.O */
M192_dc_anchor_2467:
.byte (M192_o.o.o)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_2470:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2473:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2476:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2479:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2482:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2485:
.byte (M192_o...o)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_2488:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2491:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_2494:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2497:
.byte (M192_o...o)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2500:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2503:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2506:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2509:
.byte (.....)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2512:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2515:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2518:
.byte (M192_o...o)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2521:
.byte (M192_o...o)&255
/* QL:  dc.b .OOOO */
M192_dc_anchor_2524:
.byte (.oooo)&255
/* QL:  dc.b ....O */
M192_dc_anchor_2527:
.byte (....o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2530:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2533:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2536:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_2539:
.byte (M192_ooooo)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_2542:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2545:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_2548:
.byte (.o...)&255
/* QL:  dc.b OOOOO */
M192_dc_anchor_2551:
.byte (M192_ooooo)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2554:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2557:
.byte (.....)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_2560:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2563:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2566:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_2569:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2572:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2575:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_2578:
.byte (...o.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2581:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2584:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2587:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2590:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2593:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2596:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2599:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2602:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2605:
.byte (..o..)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2608:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2611:
.byte (.....)&255
/* QL:  dc.b .O... */
M192_dc_anchor_2614:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2617:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2620:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M192_dc_anchor_2623:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2626:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M192_dc_anchor_2629:
.byte (..o..)&255
/* QL:  dc.b .O... */
M192_dc_anchor_2632:
.byte (.o...)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2635:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2638:
.byte (.....)&255
/* QL:  dc.b ..O.O */
M192_dc_anchor_2641:
.byte (..o.o)&255
/* QL:  dc.b .O.O. */
M192_dc_anchor_2644:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2647:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2650:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2653:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2656:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2659:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2662:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2665:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2668:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2671:
.byte (M192_o...o)&255
/* QL:  dc.b O.OOO */
M192_dc_anchor_2674:
.byte (M192_o.ooo)&255
/* QL:  dc.b OO..O */
M192_dc_anchor_2677:
.byte (M192_oo..o)&255
/* QL:  dc.b O.OOO */
M192_dc_anchor_2680:
.byte (M192_o.ooo)&255
/* QL:  dc.b O...O */
M192_dc_anchor_2683:
.byte (M192_o...o)&255
/* QL:  dc.b .OOO. */
M192_dc_anchor_2686:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2689:
.byte (.....)&255
/* QL:  dc.b ..... */
M192_dc_anchor_2692:
.byte (.....)&255
/* QL: M192_tb_f0x */
M192_tb_f0x:
/* QL:         end */
