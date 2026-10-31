/* ROM section tb_f1 */
.balign 2,0
/* QL: tb_f1 */
tb_f1:
/* QL:  dc.b $80,(M193_tb_f1x-tb_f1-11)/9 first character and total number less one */
M193_dc_anchor_72:
.byte (0x80)&255
.byte ((((M193_tb_f1x-tb_f1)-11)/9))&255
/* QL:  dc.b O...O */
M193_dc_anchor_76:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_79:
.byte (.....)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_82:
.byte (.oo.o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_85:
.byte (M193_o..oo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_88:
.byte (M193_o...o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_91:
.byte (M193_o..oo)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_94:
.byte (.oo.o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_97:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_100:
.byte (.....)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_103:
.byte (..o.o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_106:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_109:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_112:
.byte (.oooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_115:
.byte (M193_o...o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_118:
.byte (M193_o..oo)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_121:
.byte (.oo.o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_124:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_127:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_130:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_133:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_136:
.byte (..o..)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_139:
.byte (.oooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_142:
.byte (M193_o...o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_145:
.byte (M193_o..oo)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_148:
.byte (.oo.o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_151:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_154:
.byte (.....)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_157:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_160:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_163:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_166:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_169:
.byte (M193_ooooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_172:
.byte (M193_o....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_175:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_178:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_181:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_184:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_187:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_190:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_193:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_196:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_199:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_202:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_205:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_208:
.byte (.....)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_211:
.byte (..o.o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_214:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_217:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_220:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_223:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_226:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_229:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_232:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_235:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_238:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_241:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_244:
.byte (.oooo)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_247:
.byte (M193_o..oo)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_250:
.byte (M193_o.o.o)&255
/* QL:  dc.b OO..O */
M193_dc_anchor_253:
.byte (M193_oo..o)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_256:
.byte (M193_oooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_259:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_262:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_265:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_268:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_271:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_274:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_277:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_280:
.byte (M193_o...o)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_283:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_286:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_289:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_292:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_295:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_298:
.byte (.oooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_301:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_304:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_307:
.byte (M193_o....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_310:
.byte (.oooo)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_313:
.byte (..o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_316:
.byte (.o...)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_319:
.byte (..o.o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_322:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_325:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_328:
.byte (M193_oooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_331:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_334:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_337:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_340:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_343:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_346:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_349:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_352:
.byte (.oooo)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_355:
.byte (..o.o)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_358:
.byte (.oooo)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_361:
.byte (M193_o.o..)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_364:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_367:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_370:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_373:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_376:
.byte (.....)&255
/* QL:  dc.b .O.OO */
M193_dc_anchor_379:
.byte (.o.oo)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_382:
.byte (M193_o.o..)&255
/* QL:  dc.b O.OOO */
M193_dc_anchor_385:
.byte (M193_o.ooo)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_388:
.byte (M193_o.o..)&255
/* QL:  dc.b .O.OO */
M193_dc_anchor_391:
.byte (.o.oo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_394:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_397:
.byte (.....)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_400:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_403:
.byte (..o..)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_406:
.byte (.oo.o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_409:
.byte (M193_o..oo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_412:
.byte (M193_o...o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_415:
.byte (M193_o..oo)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_418:
.byte (.oo.o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_421:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_424:
.byte (.....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_427:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_430:
.byte (..o..)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_433:
.byte (.oo.o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_436:
.byte (M193_o..oo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_439:
.byte (M193_o...o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_442:
.byte (M193_o..oo)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_445:
.byte (.oo.o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_448:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_451:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_454:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_457:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_460:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_463:
.byte (.oooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_466:
.byte (M193_o...o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_469:
.byte (M193_o..oo)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_472:
.byte (.oo.o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_475:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_478:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_481:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_484:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_487:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_490:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_493:
.byte (M193_ooooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_496:
.byte (M193_o....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_499:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_502:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_505:
.byte (.....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_508:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_511:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_514:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_517:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_520:
.byte (M193_ooooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_523:
.byte (M193_o....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_526:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_529:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_532:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_535:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_538:
.byte (.o.o.)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_541:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_544:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_547:
.byte (M193_ooooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_550:
.byte (M193_o....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_553:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_556:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_559:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_562:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_565:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_568:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_571:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_574:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_577:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_580:
.byte (...o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_583:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_586:
.byte (.....)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_589:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_592:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_595:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_598:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_601:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_604:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_607:
.byte (...o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_610:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_613:
.byte (.....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_616:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_619:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_622:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_625:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_628:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_631:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_634:
.byte (...o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_637:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_640:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_643:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_646:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_649:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_652:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_655:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_658:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_661:
.byte (...o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_664:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_667:
.byte (.....)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_670:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_673:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_676:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_679:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_682:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_685:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_688:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_691:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_694:
.byte (.....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_697:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_700:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_703:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_706:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_709:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_712:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_715:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_718:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_721:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_724:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_727:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_730:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_733:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_736:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_739:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_742:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_745:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_748:
.byte (.....)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_751:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_754:
.byte (..o..)&255
/* QL:  dc.b O...O */
M193_dc_anchor_757:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_760:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_763:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_766:
.byte (M193_o...o)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_769:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_772:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_775:
.byte (.....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_778:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_781:
.byte (..o..)&255
/* QL:  dc.b O...O */
M193_dc_anchor_784:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_787:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_790:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_793:
.byte (M193_o...o)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_796:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_799:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_802:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_805:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_808:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_811:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_814:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_817:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_820:
.byte (M193_o...o)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_823:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_826:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_829:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_832:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_835:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_838:
.byte (M193_o...o)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_841:
.byte (M193_o.oo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_844:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_847:
.byte (M193_o...o)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_850:
.byte (M193_o.oo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_853:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_856:
.byte (M193_o....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_859:
.byte (.....)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_862:
.byte (...o.)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_865:
.byte (.oooo)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_868:
.byte (M193_o..o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_871:
.byte (M193_o..o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_874:
.byte (M193_o..o.)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_877:
.byte (.oooo)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_880:
.byte (...o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_883:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_886:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_889:
.byte (M193_o...o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_892:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_895:
.byte (..o..)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_898:
.byte (M193_ooooo)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_901:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_904:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_907:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_910:
.byte (.....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_913:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_916:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_919:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_922:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_925:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_928:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_931:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_934:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_937:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_940:
.byte (M193_o...o)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_943:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_946:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_949:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_952:
.byte (M193_ooooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_955:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_958:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_961:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_964:
.byte (.....)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_967:
.byte (..o.o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_970:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_973:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_976:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_979:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_982:
.byte (M193_ooooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_985:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_988:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_991:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_994:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_997:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1000:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1003:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1006:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1009:
.byte (M193_ooooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1012:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1015:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1018:
.byte (.....)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_1021:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1024:
.byte (..o..)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1027:
.byte (M193_ooooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1030:
.byte (M193_o....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1033:
.byte (M193_ooooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1036:
.byte (M193_o....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1039:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1042:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1045:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1048:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1051:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1054:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1057:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1060:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1063:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1066:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1069:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1072:
.byte (.....)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_1075:
.byte (..o.o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1078:
.byte (.o.o.)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1081:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1084:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1087:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1090:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1093:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1096:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1099:
.byte (.....)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_1102:
.byte (.oo.o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1105:
.byte (M193_o...o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_1108:
.byte (M193_o..oo)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_1111:
.byte (M193_o.o.o)&255
/* QL:  dc.b OO..O */
M193_dc_anchor_1114:
.byte (M193_oo..o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1117:
.byte (M193_o...o)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_1120:
.byte (M193_o.oo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1123:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1126:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1129:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1132:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1135:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1138:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1141:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1144:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1147:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1150:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1153:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1156:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1159:
.byte (M193_o...o)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1162:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1165:
.byte (M193_o....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1168:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1171:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1174:
.byte (..o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_1177:
.byte (.o...)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1180:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1183:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1186:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1189:
.byte (M193_o...o)&255
/* QL:  dc.b OO..O */
M193_dc_anchor_1192:
.byte (M193_oo..o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_1195:
.byte (M193_o.o.o)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_1198:
.byte (M193_o..oo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1201:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1204:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1207:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_1210:
.byte (.oooo)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1213:
.byte (M193_o..o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1216:
.byte (M193_o..o.)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1219:
.byte (M193_ooooo)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1222:
.byte (M193_o..o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1225:
.byte (M193_o..o.)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_1228:
.byte (M193_o..oo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1231:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1234:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_1237:
.byte (.oooo)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1240:
.byte (M193_o..o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1243:
.byte (M193_o..o.)&255
/* QL:  dc.b O..OO */
M193_dc_anchor_1246:
.byte (M193_o..oo)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1249:
.byte (M193_o..o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1252:
.byte (M193_o..o.)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_1255:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1258:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1261:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1264:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1267:
.byte (.....)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_1270:
.byte (.o..o)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_1273:
.byte (M193_o.oo.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1276:
.byte (M193_o..o.)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_1279:
.byte (M193_o.oo.)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_1282:
.byte (.o..o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1285:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1288:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1291:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1294:
.byte (M193_o...o)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1297:
.byte (M193_o....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1300:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1303:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1306:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1309:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1312:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1315:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1318:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1321:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1324:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1327:
.byte (M193_ooooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1330:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1333:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1336:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1339:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1342:
.byte (.....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1345:
.byte (M193_o....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_1348:
.byte (.o...)&255
/* QL:  dc.b .O... */
M193_dc_anchor_1351:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1354:
.byte (..o..)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_1357:
.byte (..oo.)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1360:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1363:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1366:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1369:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1372:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1375:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1378:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1381:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1384:
.byte (M193_o...o)&255
/* QL:  dc.b OO..O */
M193_dc_anchor_1387:
.byte (M193_oo..o)&255
/* QL:  dc.b O.OOO */
M193_dc_anchor_1390:
.byte (M193_o.ooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1393:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1396:
.byte (M193_o....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1399:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1402:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_1405:
.byte (.oooo)&255
/* QL:  dc.b OO.O. */
M193_dc_anchor_1408:
.byte (M193_oo.o.)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1411:
.byte (.o.o.)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1414:
.byte (.o.o.)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1417:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1420:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1423:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1426:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1429:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1432:
.byte (.ooo.)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_1435:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_1438:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_1441:
.byte (M193_o.o.o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1444:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1447:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1450:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1453:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1456:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1459:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1462:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1465:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1468:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1471:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1474:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1477:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1480:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1483:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1486:
.byte (..o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_1489:
.byte (.o...)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1492:
.byte (M193_o....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1495:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1498:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1501:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1504:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1507:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1510:
.byte (M193_o...o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_1513:
.byte (M193_o.o.o)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1516:
.byte (M193_o..o.)&255
/* QL:  dc.b .OO.O */
M193_dc_anchor_1519:
.byte (.oo.o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1522:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1525:
.byte (M193_o...o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1528:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1531:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1534:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1537:
.byte (M193_o...o)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1540:
.byte (M193_o....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1543:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1546:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1549:
.byte (.ooo.)&255
/* QL:  dc.b ....O */
M193_dc_anchor_1552:
.byte (....o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1555:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1558:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1561:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1564:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1567:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1570:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1573:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1576:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1579:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1582:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1585:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1588:
.byte (.....)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_1591:
.byte (..o.o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1594:
.byte (.o.o.)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_1597:
.byte (M193_o.o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1600:
.byte (.o.o.)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_1603:
.byte (..o.o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1606:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1609:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1612:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1615:
.byte (.....)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_1618:
.byte (M193_o.o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1621:
.byte (.o.o.)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_1624:
.byte (..o.o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1627:
.byte (.o.o.)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_1630:
.byte (M193_o.o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1633:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1636:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1639:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1642:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1645:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1648:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1651:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1654:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1657:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1660:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1663:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1666:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1669:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1672:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1675:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1678:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1681:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1684:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1687:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1690:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1693:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1696:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1699:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1702:
.byte (..o..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_1705:
.byte (.oo..)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1708:
.byte (M193_ooooo)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_1711:
.byte (.oo..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1714:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1717:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1720:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1723:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1726:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1729:
.byte (..o..)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_1732:
.byte (..oo.)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1735:
.byte (M193_ooooo)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_1738:
.byte (..oo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1741:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1744:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1747:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1750:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1753:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1756:
.byte (.ooo.)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1759:
.byte (M193_ooooo)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1762:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1765:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1768:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1771:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1774:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1777:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1780:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1783:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1786:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1789:
.byte (..o..)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1792:
.byte (M193_ooooo)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1795:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1798:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1801:
.byte (.....)&255
/* QL: * 192 minerva extended chars! */
/* QL:  dc.b ..... */
M193_dc_anchor_1805:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_1808:
.byte (M193_oooo.)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_1811:
.byte (M193_ooo..)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_1814:
.byte (M193_ooo..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_1817:
.byte (M193_o.o..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1820:
.byte (M193_o..o.)&255
/* QL:  dc.b ....O */
M193_dc_anchor_1823:
.byte (....o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1826:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1829:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1832:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_1835:
.byte (.oooo)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_1838:
.byte (..ooo)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_1841:
.byte (..ooo)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_1844:
.byte (..o.o)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_1847:
.byte (.o..o)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1850:
.byte (M193_o....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1853:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1856:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1859:
.byte (.....)&255
/* QL:  dc.b ....O */
M193_dc_anchor_1862:
.byte (....o)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1865:
.byte (M193_o..o.)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_1868:
.byte (M193_o.o..)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_1871:
.byte (M193_ooo..)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_1874:
.byte (M193_ooo..)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_1877:
.byte (M193_oooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1880:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1883:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1886:
.byte (.....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_1889:
.byte (M193_o....)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_1892:
.byte (.o..o)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_1895:
.byte (..o.o)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_1898:
.byte (..ooo)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_1901:
.byte (..ooo)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_1904:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1907:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1910:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1913:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1916:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1919:
.byte (.o.o.)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_1922:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1925:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1928:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1931:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1934:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1937:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1940:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1943:
.byte (.....)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_1946:
.byte (M193_o.oo.)&255
/* QL:  dc.b OO..O */
M193_dc_anchor_1949:
.byte (M193_oo..o)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_1952:
.byte (.o..o)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_1955:
.byte (.o..o)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_1958:
.byte (M193_o..o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_1961:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1964:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1967:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1970:
.byte (..o..)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1973:
.byte (M193_ooooo)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_1976:
.byte (M193_o.o.o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1979:
.byte (M193_ooooo)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_1982:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_1985:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1988:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_1991:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_1994:
.byte (M193_ooooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_1997:
.byte (M193_o...o)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2000:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2003:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2006:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2009:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2012:
.byte (M193_o....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2015:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2018:
.byte (.....)&255
/* QL: * 200 */
/* QL:  dc.b ..O.. */
M193_dc_anchor_2022:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2025:
.byte (.ooo.)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2028:
.byte (.ooo.)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2031:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2034:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2037:
.byte (M193_ooooo)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2040:
.byte (M193_o.o.o)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2043:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2046:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2049:
.byte (.....)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2052:
.byte (.o.o.)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2055:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2058:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2061:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2064:
.byte (M193_ooooo)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2067:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2070:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2073:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2076:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2079:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2082:
.byte (.ooo.)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2085:
.byte (.ooo.)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2088:
.byte (M193_ooooo)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2091:
.byte (.ooo.)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2094:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2097:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2100:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2103:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2106:
.byte (..o..)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2109:
.byte (M193_o.o.o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2112:
.byte (M193_ooooo)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2115:
.byte (M193_o.o.o)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2118:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2121:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2124:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2127:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2130:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2133:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2136:
.byte (.o.o.)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2139:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2142:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2145:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2148:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2151:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2154:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2157:
.byte (M193_ooooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2160:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2163:
.byte (M193_o...o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2166:
.byte (.o.o.)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2169:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2172:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2175:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2178:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2181:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2184:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2187:
.byte (.....)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2190:
.byte (.o.o.)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2193:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2196:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2199:
.byte (M193_o.o.o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2202:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2205:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2208:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2211:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2214:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2217:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2220:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2223:
.byte (M193_o...o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2226:
.byte (.o.o.)&255
/* QL:  dc.b OO.OO */
M193_dc_anchor_2229:
.byte (M193_oo.oo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2232:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2235:
.byte (.....)&255
/* QL: * 208 */
/* QL:  dc.b OOOOO */
M193_dc_anchor_2239:
.byte (M193_ooooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2242:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2245:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2248:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2251:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2254:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2257:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2260:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2263:
.byte (.....)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2266:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2269:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2272:
.byte (M193_o.o.o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2275:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2278:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2281:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2284:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2287:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2290:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_2293:
.byte (.oooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2296:
.byte (M193_o....)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_2299:
.byte (M193_o.oo.)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2302:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_2305:
.byte (M193_o.oo.)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2308:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2311:
.byte (M193_o....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_2314:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2317:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2320:
.byte (M193_ooooo)&255
/* QL:  dc.b .O... */
M193_dc_anchor_2323:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2326:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_2329:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2332:
.byte (..o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_2335:
.byte (.o...)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2338:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2341:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2344:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2347:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2350:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2353:
.byte (M193_o...o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2356:
.byte (M193_o.o.o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2359:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2362:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2365:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2368:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2371:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2374:
.byte (M193_o...o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2377:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2380:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2383:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2386:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2389:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2392:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2395:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2398:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2401:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2404:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2407:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2410:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2413:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2416:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2419:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2422:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2425:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2428:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2431:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2434:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2437:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2440:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2443:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2446:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2449:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2452:
.byte (..o..)&255
/* QL: * 216 */
/* QL:  dc.b OOOOO */
M193_dc_anchor_2456:
.byte (M193_ooooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2459:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2462:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2465:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2468:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2471:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2474:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2477:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2480:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2483:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2486:
.byte (..o..)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2489:
.byte (M193_ooooo)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2492:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2495:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2498:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2501:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2504:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2507:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2510:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2513:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2516:
.byte (.ooo.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2519:
.byte (M193_o...o)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2522:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2525:
.byte (M193_o....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2528:
.byte (.ooo.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_2531:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2534:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2537:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2540:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2543:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2546:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2549:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2552:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2555:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2558:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2561:
.byte (.....)&255
/* QL:  dc.b ...OO */
M193_dc_anchor_2564:
.byte (...oo)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_2567:
.byte (.oo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2570:
.byte (M193_o....)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_2573:
.byte (.oo..)&255
/* QL:  dc.b ...OO */
M193_dc_anchor_2576:
.byte (...oo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2579:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2582:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2585:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2588:
.byte (.....)&255
/* QL:  dc.b ....O */
M193_dc_anchor_2591:
.byte (....o)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_2594:
.byte (...o.)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2597:
.byte (M193_ooooo)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2600:
.byte (..o..)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2603:
.byte (M193_ooooo)&255
/* QL:  dc.b .O... */
M193_dc_anchor_2606:
.byte (.o...)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2609:
.byte (M193_o....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2612:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2615:
.byte (.....)&255
/* QL:  dc.b OO... */
M193_dc_anchor_2618:
.byte (M193_oo...)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_2621:
.byte (..oo.)&255
/* QL:  dc.b ....O */
M193_dc_anchor_2624:
.byte (....o)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_2627:
.byte (..oo.)&255
/* QL:  dc.b OO... */
M193_dc_anchor_2630:
.byte (M193_oo...)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2633:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2636:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2639:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2642:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2645:
.byte (.....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_2648:
.byte (.o...)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2651:
.byte (M193_o.o.o)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_2654:
.byte (...o.)&255
/* QL:  dc.b .O... */
M193_dc_anchor_2657:
.byte (.o...)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2660:
.byte (M193_o.o.o)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_2663:
.byte (...o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2666:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2669:
.byte (.....)&255
/* QL: * 224 */
/* QL:  dc.b OOOOO */
M193_dc_anchor_2673:
.byte (M193_ooooo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2676:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2679:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2682:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2685:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2688:
.byte (M193_o...o)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2691:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2694:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2697:
.byte (.....)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2700:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2703:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2706:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2709:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2712:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2715:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2718:
.byte (M193_ooooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2721:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2724:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2727:
.byte (.ooo.)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2730:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2733:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2736:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2739:
.byte (M193_ooooo)&255
/* QL:  dc.b OOOOO */
M193_dc_anchor_2742:
.byte (M193_ooooo)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2745:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2748:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2751:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2754:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2757:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2760:
.byte (M193_o...o)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_2763:
.byte (.o..o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2766:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2769:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_2772:
.byte (.o.o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_2775:
.byte (M193_o..o.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2778:
.byte (M193_o...o)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_2781:
.byte (.oo..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_2784:
.byte (M193_o..o.)&255
/* QL:  dc.b ....O */
M193_dc_anchor_2787:
.byte (....o)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_2790:
.byte (..ooo)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_2793:
.byte (.o..o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2796:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2799:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2802:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2805:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2808:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2811:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_2814:
.byte (.oooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2817:
.byte (M193_o....)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_2820:
.byte (M193_oooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2823:
.byte (M193_o....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_2826:
.byte (.oooo)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2829:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2832:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_2835:
.byte (M193_oooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2838:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_2841:
.byte (M193_ooo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_2844:
.byte (M193_o....)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_2847:
.byte (M193_o.oo.)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_2850:
.byte (M193_o.o.o)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_2853:
.byte (..oo.)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_2856:
.byte (..o.o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2859:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2862:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2865:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2868:
.byte (M193_o...o)&255
/* QL:  dc.b OO..O */
M193_dc_anchor_2871:
.byte (M193_oo..o)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_2874:
.byte (..o.o)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_2877:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_2880:
.byte (...o.)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_2883:
.byte (.oo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2886:
.byte (.....)&255
/* QL: * 232 */
/* QL:  dc.b .O.O. */
M193_dc_anchor_2890:
.byte (.o.o.)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_2893:
.byte (.oo..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_2896:
.byte (.o...)&255
/* QL:  dc.b OO... */
M193_dc_anchor_2899:
.byte (M193_oo...)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_2902:
.byte (.ooo.)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_2905:
.byte (.o..o)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_2908:
.byte (.o..o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2911:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2914:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2917:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2920:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2923:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2926:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2929:
.byte (..o..)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_2932:
.byte (..o.o)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_2935:
.byte (...o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2938:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2941:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2944:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2947:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2950:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2953:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2956:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2959:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2962:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2965:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_2968:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2971:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2974:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2977:
.byte (M193_o...o)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_2980:
.byte (M193_o..o.)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_2983:
.byte (M193_ooo..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_2986:
.byte (M193_o..o.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_2989:
.byte (M193_o...o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2992:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_2995:
.byte (.....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_2998:
.byte (.o...)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3001:
.byte (.o...)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3004:
.byte (.o..o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3007:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3010:
.byte (..o..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3013:
.byte (.o.o.)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_3016:
.byte (M193_o.oo.)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_3019:
.byte (.oooo)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3022:
.byte (...o.)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3025:
.byte (.o...)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3028:
.byte (.o...)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3031:
.byte (.o..o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3034:
.byte (.o.o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3037:
.byte (..o..)&255
/* QL:  dc.b .O.OO */
M193_dc_anchor_3040:
.byte (.o.oo)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3043:
.byte (M193_o...o)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3046:
.byte (...o.)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3049:
.byte (..ooo)&255
/* QL:  dc.b OO... */
M193_dc_anchor_3052:
.byte (M193_oo...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3055:
.byte (..o..)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3058:
.byte (.o..o)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3061:
.byte (..oo.)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3064:
.byte (M193_ooo..)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3067:
.byte (.o.o.)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_3070:
.byte (M193_o.oo.)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_3073:
.byte (.oooo)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3076:
.byte (...o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3079:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3082:
.byte (.....)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3085:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3088:
.byte (M193_o...o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_3091:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_3094:
.byte (M193_o.o.o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3097:
.byte (.o.o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3100:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3103:
.byte (.....)&255
/* QL: * 240 */
/* QL:  dc.b ..O.. */
M193_dc_anchor_3107:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3110:
.byte (..o..)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_3113:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_3116:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_3119:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_3122:
.byte (M193_o.o.o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_3125:
.byte (.ooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3128:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3131:
.byte (..o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3134:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3137:
.byte (..o..)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_3140:
.byte (M193_oooo.)&255
/* QL:  dc.b ....O */
M193_dc_anchor_3143:
.byte (....o)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_3146:
.byte (M193_oooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3149:
.byte (..o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3152:
.byte (.o...)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3155:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3158:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3161:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3164:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3167:
.byte (..oo.)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3170:
.byte (.o..o)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3173:
.byte (.o..o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_3176:
.byte (.ooo.)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3179:
.byte (.o...)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3182:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3185:
.byte (M193_o....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3188:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3191:
.byte (.....)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3194:
.byte (..ooo)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3197:
.byte (.o.o.)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3200:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3203:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_3206:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3209:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3212:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3215:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3218:
.byte (.....)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_3221:
.byte (.oooo)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3224:
.byte (M193_o.o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3227:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3230:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3233:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3236:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3239:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3242:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3245:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3248:
.byte (M193_o...o)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3251:
.byte (.o..o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3254:
.byte (M193_o...o)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3257:
.byte (M193_o...o)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_3260:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3263:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3266:
.byte (.....)&255
/* QL:  dc.b ....O */
M193_dc_anchor_3269:
.byte (....o)&255
/* QL:  dc.b ....O */
M193_dc_anchor_3272:
.byte (....o)&255
/* QL:  dc.b ....O */
M193_dc_anchor_3275:
.byte (....o)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3278:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3281:
.byte (...o.)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3284:
.byte (.o.o.)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3287:
.byte (M193_o.o..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3290:
.byte (M193_o.o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3293:
.byte (..o..)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3296:
.byte (M193_o...o)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3299:
.byte (.o..o)&255
/* QL:  dc.b OO..O */
M193_dc_anchor_3302:
.byte (M193_oo..o)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3305:
.byte (.o.o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_3308:
.byte (M193_o..o.)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3311:
.byte (.o.o.)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3314:
.byte (M193_o.o..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3317:
.byte (M193_o.o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3320:
.byte (..o..)&255
/* QL: * 248 */
/* QL:  dc.b .O... */
M193_dc_anchor_3324:
.byte (.o...)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_3327:
.byte (.oooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3330:
.byte (M193_o....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_3333:
.byte (.ooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3336:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3339:
.byte (M193_o....)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3342:
.byte (.oo..)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3345:
.byte (...o.)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3348:
.byte (.oo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3351:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3354:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3357:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3360:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3363:
.byte (.....)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_3366:
.byte (M193_o.o.o)&255
/* QL:  dc.b O.O.O */
M193_dc_anchor_3369:
.byte (M193_o.o.o)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3372:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3375:
.byte (.....)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3378:
.byte (.o..o)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3381:
.byte (..ooo)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3384:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3387:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3390:
.byte (..o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3393:
.byte (.o...)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_3396:
.byte (.ooo.)&255
/* QL:  dc.b ....O */
M193_dc_anchor_3399:
.byte (....o)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3402:
.byte (..oo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3405:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3408:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3411:
.byte (..oo.)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_3414:
.byte (.oooo)&255
/* QL:  dc.b OO.OO */
M193_dc_anchor_3417:
.byte (M193_oo.oo)&255
/* QL:  dc.b OO... */
M193_dc_anchor_3420:
.byte (M193_oo...)&255
/* QL:  dc.b OO... */
M193_dc_anchor_3423:
.byte (M193_oo...)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3426:
.byte (.oo..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3429:
.byte (.oo..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3432:
.byte (.oo..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3435:
.byte (.oo..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3438:
.byte (.oo..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3441:
.byte (.oo..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3444:
.byte (.oo..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3447:
.byte (.oo..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3450:
.byte (.oo..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3453:
.byte (.oo..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3456:
.byte (.oo..)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3459:
.byte (..oo.)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3462:
.byte (..oo.)&255
/* QL:  dc.b ...OO */
M193_dc_anchor_3465:
.byte (...oo)&255
/* QL:  dc.b ...OO */
M193_dc_anchor_3468:
.byte (...oo)&255
/* QL:  dc.b OO.OO */
M193_dc_anchor_3471:
.byte (M193_oo.oo)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_3474:
.byte (M193_oooo.)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3477:
.byte (.oo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3480:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3483:
.byte (.....)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3486:
.byte (M193_o...o)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3489:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3492:
.byte (...o.)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3495:
.byte (.o...)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3498:
.byte (M193_o...o)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3501:
.byte (...o.)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3504:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3507:
.byte (..o..)&255
/* QL:  dc.b O...O */
M193_dc_anchor_3510:
.byte (M193_o...o)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_3513:
.byte (M193_o..o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3516:
.byte (..o..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_3519:
.byte (M193_o..o.)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3522:
.byte (.o..o)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3525:
.byte (..o..)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3528:
.byte (.o..o)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_3531:
.byte (M193_o..o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3534:
.byte (..o..)&255
/* QL:  dc.b .O..O */
M193_dc_anchor_3537:
.byte (.o..o)&255
/* QL: * 0 */
/* QL:  dc.b OO.O. */
M193_dc_anchor_3541:
.byte (M193_oo.o.)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_3544:
.byte (M193_o.oo.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_3547:
.byte (M193_o..o.)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3550:
.byte (..o.o)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3553:
.byte (..o.o)&255
/* QL:  dc.b O.OOO */
M193_dc_anchor_3556:
.byte (M193_o.ooo)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3559:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3562:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3565:
.byte (M193_ooo..)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_3568:
.byte (M193_oooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3571:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3574:
.byte (M193_ooo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3577:
.byte (M193_o....)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_3580:
.byte (M193_o..o.)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3583:
.byte (..oo.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3586:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3589:
.byte (...o.)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3592:
.byte (..ooo)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_3595:
.byte (M193_oooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3598:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3601:
.byte (M193_ooo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3604:
.byte (M193_o....)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_3607:
.byte (M193_o..o.)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3610:
.byte (..o.o)&255
/* QL:  dc.b ....O */
M193_dc_anchor_3613:
.byte (....o)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3616:
.byte (...o.)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3619:
.byte (..ooo)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_3622:
.byte (M193_oooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3625:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3628:
.byte (M193_ooo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3631:
.byte (M193_o....)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_3634:
.byte (M193_o.oo.)&255
/* QL:  dc.b ....O */
M193_dc_anchor_3637:
.byte (....o)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3640:
.byte (..oo.)&255
/* QL:  dc.b ....O */
M193_dc_anchor_3643:
.byte (....o)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3646:
.byte (..oo.)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_3649:
.byte (M193_oooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3652:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3655:
.byte (M193_ooo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3658:
.byte (M193_o....)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_3661:
.byte (M193_o..o.)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3664:
.byte (..oo.)&255
/* QL:  dc.b .O.O. */
M193_dc_anchor_3667:
.byte (.o.o.)&255
/* QL:  dc.b .OOOO */
M193_dc_anchor_3670:
.byte (.oooo)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3673:
.byte (...o.)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_3676:
.byte (M193_oooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3679:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3682:
.byte (M193_ooo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3685:
.byte (M193_o....)&255
/* QL:  dc.b O.OOO */
M193_dc_anchor_3688:
.byte (M193_o.ooo)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3691:
.byte (..o..)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3694:
.byte (..ooo)&255
/* QL:  dc.b ....O */
M193_dc_anchor_3697:
.byte (....o)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3700:
.byte (..ooo)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3703:
.byte (.o...)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3706:
.byte (M193_o.o..)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3709:
.byte (M193_ooo..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3712:
.byte (M193_o.o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3715:
.byte (.....)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3718:
.byte (..o.o)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3721:
.byte (..oo.)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3724:
.byte (..oo.)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3727:
.byte (..o.o)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3730:
.byte (..o..)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3733:
.byte (..oo.)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3736:
.byte (..o.o)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3739:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3742:
.byte (..o..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3745:
.byte (.oo..)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3748:
.byte (M193_ooo..)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3751:
.byte (M193_ooo..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3754:
.byte (.o...)&255
/* QL: * 8 */
/* QL:  dc.b OO... */
M193_dc_anchor_3758:
.byte (M193_oo...)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3761:
.byte (M193_o.o..)&255
/* QL:  dc.b OO... */
M193_dc_anchor_3764:
.byte (M193_oo...)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3767:
.byte (M193_o.o..)&255
/* QL:  dc.b OO.OO */
M193_dc_anchor_3770:
.byte (M193_oo.oo)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3773:
.byte (..o..)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3776:
.byte (...o.)&255
/* QL:  dc.b ....O */
M193_dc_anchor_3779:
.byte (....o)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3782:
.byte (..oo.)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3785:
.byte (M193_o.o..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3788:
.byte (M193_o.o..)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3791:
.byte (M193_ooo..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3794:
.byte (M193_o.o..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3797:
.byte (M193_o.o..)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3800:
.byte (..ooo)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3803:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3806:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3809:
.byte (...o.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3812:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3815:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3818:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3821:
.byte (M193_ooo..)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3824:
.byte (..ooo)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3827:
.byte (..o..)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3830:
.byte (..oo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3833:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3836:
.byte (..o..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3839:
.byte (M193_o.o..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3842:
.byte (M193_o.o..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_3845:
.byte (M193_o.o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3848:
.byte (.o...)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3851:
.byte (..ooo)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3854:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3857:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3860:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3863:
.byte (...o.)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_3866:
.byte (M193_ooo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3869:
.byte (M193_o....)&255
/* QL:  dc.b OO... */
M193_dc_anchor_3872:
.byte (M193_oo...)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3875:
.byte (M193_o....)&255
/* QL:  dc.b O.OOO */
M193_dc_anchor_3878:
.byte (M193_o.ooo)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3881:
.byte (..o..)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3884:
.byte (..oo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3887:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3890:
.byte (..o..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3893:
.byte (.oo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3896:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3899:
.byte (M193_o....)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3902:
.byte (.oo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3905:
.byte (.....)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3908:
.byte (..oo.)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3911:
.byte (..o.o)&255
/* QL:  dc.b ..OO. */
M193_dc_anchor_3914:
.byte (..oo.)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3917:
.byte (..o.o)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3920:
.byte (.oo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3923:
.byte (M193_o....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3926:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3929:
.byte (..o..)&255
/* QL:  dc.b OO.O. */
M193_dc_anchor_3932:
.byte (M193_oo.o.)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3935:
.byte (..o.o)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3938:
.byte (..o.o)&255
/* QL:  dc.b ..O.O */
M193_dc_anchor_3941:
.byte (..o.o)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3944:
.byte (...o.)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3947:
.byte (.oo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_3950:
.byte (M193_o....)&255
/* QL:  dc.b .O... */
M193_dc_anchor_3953:
.byte (.o...)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_3956:
.byte (..o..)&255
/* QL:  dc.b OO... */
M193_dc_anchor_3959:
.byte (M193_oo...)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3962:
.byte (..ooo)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3965:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_3968:
.byte (...o.)&255
/* QL:  dc.b ..OOO */
M193_dc_anchor_3971:
.byte (..ooo)&255
/* QL: tb_hex ; * 16 */
tb_hex:
/* QL:  dc.b .OO.. */
M193_dc_anchor_3976:
.byte (.oo..)&255
/* QL:  dc.b O.OO. */
M193_dc_anchor_3979:
.byte (M193_o.oo.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_3982:
.byte (M193_o..o.)&255
/* QL:  dc.b OO.O. */
M193_dc_anchor_3985:
.byte (M193_oo.o.)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_3988:
.byte (.oo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3991:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3994:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_3997:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4000:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_4003:
.byte (..o..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_4006:
.byte (.oo..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_4009:
.byte (..o..)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_4012:
.byte (..o..)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_4015:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4018:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4021:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4024:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4027:
.byte (.....)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_4030:
.byte (.oo..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4033:
.byte (M193_o..o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_4036:
.byte (..o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_4039:
.byte (.o...)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_4042:
.byte (M193_oooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4045:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4048:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4051:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4054:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_4057:
.byte (M193_oooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_4060:
.byte (..o..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_4063:
.byte (.oo..)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_4066:
.byte (...o.)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4069:
.byte (M193_ooo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4072:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4075:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4078:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4081:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_4084:
.byte (..o..)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_4087:
.byte (.oo..)&255
/* QL:  dc.b O.O.. */
M193_dc_anchor_4090:
.byte (M193_o.o..)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_4093:
.byte (M193_oooo.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_4096:
.byte (..o..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4099:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4102:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4105:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4108:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_4111:
.byte (M193_oooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_4114:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4117:
.byte (M193_ooo..)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_4120:
.byte (...o.)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4123:
.byte (M193_ooo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4126:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4129:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4132:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4135:
.byte (.....)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_4138:
.byte (..o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_4141:
.byte (.o...)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4144:
.byte (M193_ooo..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4147:
.byte (M193_o..o.)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_4150:
.byte (.oo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4153:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4156:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4159:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4162:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_4165:
.byte (M193_oooo.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_4168:
.byte (...o.)&255
/* QL:  dc.b ..O.. */
M193_dc_anchor_4171:
.byte (..o..)&255
/* QL:  dc.b .O... */
M193_dc_anchor_4174:
.byte (.o...)&255
/* QL:  dc.b O.... */
M193_dc_anchor_4177:
.byte (M193_o....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4180:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4183:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4186:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4189:
.byte (.....)&255
/* QL: * 24 */
/* QL:  dc.b .OO.. */
M193_dc_anchor_4193:
.byte (.oo..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4196:
.byte (M193_o..o.)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_4199:
.byte (.oo..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4202:
.byte (M193_o..o.)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_4205:
.byte (.oo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4208:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4211:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4214:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4217:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_4220:
.byte (.ooo.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4223:
.byte (M193_o..o.)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_4226:
.byte (.ooo.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_4229:
.byte (...o.)&255
/* QL:  dc.b ...O. */
M193_dc_anchor_4232:
.byte (...o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4235:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4238:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4241:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4244:
.byte (.....)&255
/* QL:  dc.b .OO.. */
M193_dc_anchor_4247:
.byte (.oo..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4250:
.byte (M193_o..o.)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_4253:
.byte (M193_oooo.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4256:
.byte (M193_o..o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4259:
.byte (M193_o..o.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4262:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4265:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4268:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4271:
.byte (.....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4274:
.byte (M193_ooo..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4277:
.byte (M193_o..o.)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4280:
.byte (M193_ooo..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4283:
.byte (M193_o..o.)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4286:
.byte (M193_ooo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4289:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4292:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4295:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4298:
.byte (.....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_4301:
.byte (.ooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_4304:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_4307:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_4310:
.byte (M193_o....)&255
/* QL:  dc.b .OOO. */
M193_dc_anchor_4313:
.byte (.ooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4316:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4319:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4322:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4325:
.byte (.....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4328:
.byte (M193_ooo..)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4331:
.byte (M193_o..o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4334:
.byte (M193_o..o.)&255
/* QL:  dc.b O..O. */
M193_dc_anchor_4337:
.byte (M193_o..o.)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4340:
.byte (M193_ooo..)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4343:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4346:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4349:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4352:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_4355:
.byte (M193_oooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_4358:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4361:
.byte (M193_ooo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_4364:
.byte (M193_o....)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_4367:
.byte (M193_oooo.)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4370:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4373:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4376:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4379:
.byte (.....)&255
/* QL:  dc.b OOOO. */
M193_dc_anchor_4382:
.byte (M193_oooo.)&255
/* QL:  dc.b O.... */
M193_dc_anchor_4385:
.byte (M193_o....)&255
/* QL:  dc.b OOO.. */
M193_dc_anchor_4388:
.byte (M193_ooo..)&255
/* QL:  dc.b O.... */
M193_dc_anchor_4391:
.byte (M193_o....)&255
/* QL:  dc.b O.... */
M193_dc_anchor_4394:
.byte (M193_o....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4397:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4400:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4403:
.byte (.....)&255
/* QL:  dc.b ..... */
M193_dc_anchor_4406:
.byte (.....)&255
/* QL: * This last one included again for sys_ramt */
/* QL: M193_tb_f1x */
M193_tb_f1x:
/* QL:         end */
