# Native i386 byte-register allocation of reference/src/dust_generated.s.
# Reproduce with tools/port-dust.py. ESI=state, EDI=screen, EBP=index scratch.
.text
.globl dust_original_init
.globl dust_original_step
.globl dust_original_flip
.globl dust_original_redraw
dust_original_init:
 jmp dust_i545
dust_original_step:
 jmp dust_i14
dust_original_flip:
 jmp dust_i0
dust_original_redraw:
 jmp dust_i585
dust_i0:
 movb $10,%cl
dust_i1:
 movzbl %cl,%ebp
 movb 19(%esi,%ebp),%bl
dust_i2:
 movzbl %cl,%ebp
 movb 0(%esi,%ebp),%al
dust_i3:
 movb %al,138(%esi)
dust_i4:
 movzbl %cl,%ebp
 movb %al,19(%esi,%ebp)
dust_i5:
 movb %bl,%al
dust_i6:
 movb %al,136(%esi)
dust_i7:
 movzbl %cl,%ebp
 movb %al,0(%esi,%ebp)
dust_i8:
 movzbl %cl,%ebp
 movb 38(%esi,%ebp),%al
 movb %al,%dh
dust_i9:
 movb %al,140(%esi)
dust_i10:
 call dust_i564
dust_i11:
 subb $1,%cl
 movb %cl,%dh
dust_i12:
 testb %dh,%dh
 jne dust_i1
dust_i13:
 ret
dust_i14:
 movb 162(%esi),%bl
 movb %bl,%dh
dust_i15:
 testb %dh,%dh
 je dust_i20
dust_i16:
 subb $1,%bl
 movb %bl,%dh
dust_i17:
 testb %dh,%dh
 jne dust_i19
dust_i18:
 jmp dust_i126
dust_i19:
 jmp dust_i243
dust_i20:
 movb $10,%cl
 movb %cl,%dh
dust_i21:
 call dust_i496
dust_i22:
 movb 122(%esi),%al
dust_i23:
 movb 118(%esi),%ah
 shrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i24:
 shrb $1,%dl
 rcrb $1,%al
dust_i25:
 movb 118(%esi),%ah
 shrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i26:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i27:
 orb $1,%al
dust_i28:
 movb %al,120(%esi)
dust_i29:
 movzbl %cl,%ebp
 movb 95(%esi,%ebp),%al
dust_i30:
 movb 156(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
 setae %dl
 andb $1,%dl
dust_i31:
 movzbl %cl,%ebp
 movb %al,95(%esi,%ebp)
dust_i32:
 movzbl %cl,%ebp
 movb 38(%esi,%ebp),%al
dust_i33:
 movb %al,140(%esi)
dust_i34:
 movb 157(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
 setae %dl
 andb $1,%dl
 movb %al,%dh
dust_i35:
 movzbl %cl,%ebp
 movb %al,38(%esi,%ebp)
dust_i36:
 call dust_i408
dust_i37:
 movb %al,135(%esi)
dust_i38:
 movb 118(%esi),%al
dust_i39:
 movzbl %cl,%ebp
 movb 76(%esi,%ebp),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i40:
 movb %al,134(%esi)
dust_i41:
 movb %al,122(%esi)
dust_i42:
 movb 138(%esi),%al
dust_i43:
 movb 135(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i44:
 movb %al,135(%esi)
dust_i45:
 movb %al,124(%esi)
dust_i46:
 movzbl %cl,%ebp
 movb 0(%esi,%ebp),%al
 movb %al,%dh
dust_i47:
 movb %al,136(%esi)
dust_i48:
 call dust_i410
dust_i49:
 movb %al,133(%esi)
dust_i50:
 movb 118(%esi),%al
dust_i51:
 movzbl %cl,%ebp
 movb 57(%esi,%ebp),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i52:
 movb %al,132(%esi)
dust_i53:
 movb 136(%esi),%al
dust_i54:
 movb 133(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i55:
 movb %al,133(%esi)
dust_i56:
 movb 149(%esi),%ah
 xorb %ah,%al
 movb %al,%dh
dust_i57:
 call dust_i363
dust_i58:
 call dust_i462
dust_i59:
 movb %al,135(%esi)
dust_i60:
 movb %bl,134(%esi)
dust_i61:
 movb 148(%esi),%ah
 xorb %ah,%al
 movb %al,%dh
dust_i62:
 call dust_i359
dust_i63:
 call dust_i462
dust_i64:
 movb %al,133(%esi)
dust_i65:
 movb %bl,132(%esi)
dust_i66:
 movb 150(%esi),%bl
dust_i67:
 movb 135(%esi),%al
dust_i68:
 movb 153(%esi),%ah
 xorb %ah,%al
 movb %al,%dh
dust_i69:
 call dust_i364
dust_i70:
 movb %al,120(%esi)
dust_i71:
 call dust_i429
dust_i72:
 movb 118(%esi),%ah
 shlb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i73:
 shrb $1,%dl
 rclb $1,%al
 setb %dl
 andb $1,%dl
dust_i74:
 movb %al,126(%esi)
dust_i75:
 movb $0,%al
dust_i76:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i77:
 orb 126(%esi),%al
 movb %al,%dh
dust_i78:
 call dust_i462
dust_i79:
 movb %al,133(%esi)
dust_i80:
 movb %bl,%al
dust_i81:
 movzbl %cl,%ebp
 movb %al,57(%esi,%ebp)
dust_i82:
 movb 134(%esi),%al
dust_i83:
 movb %al,122(%esi)
dust_i84:
 movb 135(%esi),%al
dust_i85:
 movb %al,124(%esi)
dust_i86:
 movb $0,%al
dust_i87:
 movb %al,118(%esi)
dust_i88:
 movb 144(%esi),%al
dust_i89:
 xorb $128,%al
 movb %al,%dh
dust_i90:
 call dust_i560
dust_i91:
 movb 133(%esi),%al
dust_i92:
 movb %al,136(%esi)
dust_i93:
 movzbl %cl,%ebp
 movb %al,0(%esi,%ebp)
dust_i94:
 andb $127,%al
dust_i95:
 movb %al,%ah
 subb $120,%ah
 setae %dl
 andb $1,%dl
 movb %ah,%dh
dust_i96:
 testb %dl,%dl
 jne dust_i112
dust_i97:
 movb 135(%esi),%al
dust_i98:
 movzbl %cl,%ebp
 movb %al,19(%esi,%ebp)
dust_i99:
 movb %al,138(%esi)
dust_i100:
 andb $127,%al
dust_i101:
 movb %al,%ah
 subb $120,%ah
 setae %dl
 andb $1,%dl
 movb %ah,%dh
dust_i102:
 testb %dl,%dl
 jne dust_i112
dust_i103:
 movzbl %cl,%ebp
 movb 38(%esi,%ebp),%al
dust_i104:
 movb %al,%ah
 subb $16,%ah
 setae %dl
 andb $1,%dl
 movb %ah,%dh
dust_i105:
 testb %dl,%dl
 je dust_i112
dust_i106:
 movb %al,140(%esi)
dust_i107:
 call dust_i564
dust_i108:
 subb $1,%cl
 movb %cl,%dh
dust_i109:
 testb %dh,%dh
 je dust_i111
dust_i110:
 jmp dust_i21
dust_i111:
 ret
dust_i112:
 call dust_i533
dust_i113:
 orb $4,%al
 movb %al,%dh
dust_i114:
 movb %al,138(%esi)
dust_i115:
 movzbl %cl,%ebp
 movb %al,19(%esi,%ebp)
dust_i116:
 call dust_i533
dust_i117:
 orb $8,%al
 movb %al,%dh
dust_i118:
 movb %al,136(%esi)
dust_i119:
 movzbl %cl,%ebp
 movb %al,0(%esi,%ebp)
dust_i120:
 call dust_i533
dust_i121:
 orb $144,%al
dust_i122:
 movzbl %cl,%ebp
 movb %al,38(%esi,%ebp)
dust_i123:
 movb %al,140(%esi)
dust_i124:
 movb 138(%esi),%al
 movb %al,%dh
dust_i125:
 jmp dust_i107
dust_i126:
 movb $10,%cl
 movb %cl,%dh
dust_i127:
 call dust_i496
dust_i128:
 movb 122(%esi),%al
dust_i129:
 movb 118(%esi),%ah
 shrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i130:
 shrb $1,%dl
 rcrb $1,%al
dust_i131:
 movb 118(%esi),%ah
 shrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i132:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i133:
 orb $1,%al
dust_i134:
 movb %al,120(%esi)
dust_i135:
 movzbl %cl,%ebp
 movb 0(%esi,%ebp),%al
 movb %al,%dh
dust_i136:
 movb %al,136(%esi)
dust_i137:
 call dust_i410
dust_i138:
 movb %al,133(%esi)
dust_i139:
 movzbl %cl,%ebp
 movb 57(%esi,%ebp),%al
dust_i140:
 movb 118(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
 setae %dl
 andb $1,%dl
dust_i141:
 movb %al,132(%esi)
dust_i142:
 movb 136(%esi),%al
dust_i143:
 movb 133(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
 setae %dl
 andb $1,%dl
 movb %al,%dh
dust_i144:
 movb %al,133(%esi)
dust_i145:
 call dust_i408
dust_i146:
 movb %al,135(%esi)
dust_i147:
 movzbl %cl,%ebp
 movb 76(%esi,%ebp),%al
dust_i148:
 movb 118(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
 setae %dl
 andb $1,%dl
dust_i149:
 movb %al,134(%esi)
dust_i150:
 movb %al,122(%esi)
dust_i151:
 movb 138(%esi),%al
dust_i152:
 movb 135(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
 setae %dl
 andb $1,%dl
dust_i153:
 movb %al,135(%esi)
dust_i154:
 movb %al,124(%esi)
dust_i155:
 movzbl %cl,%ebp
 movb 95(%esi,%ebp),%al
dust_i156:
 movb 156(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i157:
 movzbl %cl,%ebp
 movb %al,95(%esi,%ebp)
dust_i158:
 movzbl %cl,%ebp
 movb 38(%esi,%ebp),%al
dust_i159:
 movb %al,140(%esi)
dust_i160:
 movb 157(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i161:
 movzbl %cl,%ebp
 movb %al,38(%esi,%ebp)
dust_i162:
 movb 133(%esi),%al
dust_i163:
 movb 148(%esi),%ah
 xorb %ah,%al
 movb %al,%dh
dust_i164:
 call dust_i363
dust_i165:
 call dust_i462
dust_i166:
 movb %al,135(%esi)
dust_i167:
 movb %bl,134(%esi)
dust_i168:
 movb 149(%esi),%ah
 xorb %ah,%al
 movb %al,%dh
dust_i169:
 call dust_i359
dust_i170:
 call dust_i462
dust_i171:
 movb %al,133(%esi)
dust_i172:
 movb %bl,132(%esi)
dust_i173:
 movb 135(%esi),%al
dust_i174:
 movb 153(%esi),%ah
 xorb %ah,%al
dust_i175:
 movb 150(%esi),%bl
 movb %bl,%dh
dust_i176:
 call dust_i364
dust_i177:
 movb %al,120(%esi)
dust_i178:
 movb 133(%esi),%al
dust_i179:
 movb %al,124(%esi)
dust_i180:
 xorb $128,%al
 movb %al,%dh
dust_i181:
 call dust_i431
dust_i182:
 movb 118(%esi),%ah
 shlb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i183:
 shrb $1,%dl
 rclb $1,%al
 setb %dl
 andb $1,%dl
dust_i184:
 movb %al,126(%esi)
dust_i185:
 movb $0,%al
dust_i186:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i187:
 orb 126(%esi),%al
 movb %al,%dh
dust_i188:
 call dust_i462
dust_i189:
 movb %al,133(%esi)
dust_i190:
 movb %bl,%al
dust_i191:
 movzbl %cl,%ebp
 movb %al,57(%esi,%ebp)
dust_i192:
 movb 134(%esi),%al
dust_i193:
 movb %al,122(%esi)
dust_i194:
 movb 135(%esi),%al
dust_i195:
 movb %al,124(%esi)
dust_i196:
 movb $0,%al
dust_i197:
 movb %al,118(%esi)
dust_i198:
 movb 144(%esi),%al
 movb %al,%dh
dust_i199:
 call dust_i560
dust_i200:
 movb 133(%esi),%al
dust_i201:
 movb %al,136(%esi)
dust_i202:
 movzbl %cl,%ebp
 movb %al,0(%esi,%ebp)
dust_i203:
 movb 135(%esi),%al
dust_i204:
 movzbl %cl,%ebp
 movb %al,19(%esi,%ebp)
dust_i205:
 movb %al,138(%esi)
dust_i206:
 andb $127,%al
dust_i207:
 movb %al,%ah
 subb $110,%ah
 setae %dl
 andb $1,%dl
 movb %ah,%dh
dust_i208:
 testb %dl,%dl
 jne dust_i218
dust_i209:
 movzbl %cl,%ebp
 movb 38(%esi,%ebp),%al
dust_i210:
 movb %al,%ah
 subb $160,%ah
 setae %dl
 andb $1,%dl
 movb %ah,%dh
dust_i211:
 testb %dl,%dl
 jne dust_i218
dust_i212:
 movb %al,140(%esi)
dust_i213:
 call dust_i564
dust_i214:
 subb $1,%cl
 movb %cl,%dh
dust_i215:
 testb %dh,%dh
 je dust_i217
dust_i216:
 jmp dust_i127
dust_i217:
 ret
dust_i218:
 call dust_i533
dust_i219:
 andb $127,%al
dust_i220:
 movb $10,%ah
 shrb $1,%dl
 adcb %ah,%al
dust_i221:
 movzbl %cl,%ebp
 movb %al,38(%esi,%ebp)
dust_i222:
 movb %al,140(%esi)
dust_i223:
 shrb $1,%al
 setb %dl
 andb $1,%dl
 movb %al,%dh
dust_i224:
 testb %dl,%dl
 jne dust_i234
dust_i225:
 shrb $1,%al
 setb %dl
 andb $1,%dl
dust_i226:
 movb $252,%al
dust_i227:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
 movb %al,%dh
dust_i228:
 movb %al,136(%esi)
dust_i229:
 movzbl %cl,%ebp
 movb %al,0(%esi,%ebp)
dust_i230:
 call dust_i533
dust_i231:
 movb %al,138(%esi)
dust_i232:
 movzbl %cl,%ebp
 movb %al,19(%esi,%ebp)
dust_i233:
 jmp dust_i213
dust_i234:
 call dust_i533
dust_i235:
 movb %al,136(%esi)
dust_i236:
 movzbl %cl,%ebp
 movb %al,0(%esi,%ebp)
dust_i237:
 shrb $1,%al
 setb %dl
 andb $1,%dl
dust_i238:
 movb $230,%al
dust_i239:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
 movb %al,%dh
dust_i240:
 movb %al,138(%esi)
dust_i241:
 movzbl %cl,%ebp
 movb %al,19(%esi,%ebp)
dust_i242:
 testb %dh,%dh
 jne dust_i213
dust_i243:
 movb $0,%al
dust_i244:
 movb %bl,%ah
 subb $2,%ah
 setae %dl
 andb $1,%dl
dust_i245:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i246:
 movb %al,158(%esi)
dust_i247:
 xorb $128,%al
 movb %al,%dh
dust_i248:
 movb %al,160(%esi)
dust_i249:
 call dust_i325
dust_i250:
 movb $10,%cl
dust_i251:
 movzbl %cl,%ebp
 movb 38(%esi,%ebp),%al
dust_i252:
 movb %al,140(%esi)
dust_i253:
 shrb $1,%al
dust_i254:
 shrb $1,%al
dust_i255:
 shrb $1,%al
 setb %dl
 andb $1,%dl
 movb %al,%dh
dust_i256:
 call dust_i497
dust_i257:
 movb 118(%esi),%al
dust_i258:
 movb 160(%esi),%ah
 xorb %ah,%al
dust_i259:
 movb %al,124(%esi)
dust_i260:
 movzbl %cl,%ebp
 movb 57(%esi,%ebp),%al
dust_i261:
 movb %al,118(%esi)
dust_i262:
 movzbl %cl,%ebp
 movb 0(%esi,%ebp),%al
 movb %al,%dh
dust_i263:
 movb %al,136(%esi)
dust_i264:
 call dust_i462
dust_i265:
 movb %al,124(%esi)
dust_i266:
 movb %bl,122(%esi)
dust_i267:
 movzbl %cl,%ebp
 movb 19(%esi,%ebp),%al
dust_i268:
 movb %al,138(%esi)
dust_i269:
 movb 152(%esi),%ah
 xorb %ah,%al
dust_i270:
 movb 150(%esi),%bl
 movb %bl,%dh
dust_i271:
 call dust_i364
dust_i272:
 call dust_i462
dust_i273:
 movb %bl,132(%esi)
dust_i274:
 movb %al,133(%esi)
dust_i275:
 movzbl %cl,%ebp
 movb 76(%esi,%ebp),%bl
dust_i276:
 movb %bl,122(%esi)
dust_i277:
 movb 138(%esi),%bl
dust_i278:
 movb %bl,124(%esi)
dust_i279:
 movb 150(%esi),%bl
dust_i280:
 movb 153(%esi),%ah
 xorb %ah,%al
 movb %al,%dh
dust_i281:
 call dust_i364
dust_i282:
 call dust_i462
dust_i283:
 movb %bl,134(%esi)
dust_i284:
 movb %al,135(%esi)
dust_i285:
 movb 146(%esi),%bl
dust_i286:
 movb 148(%esi),%ah
 xorb %ah,%al
 movb %al,%dh
dust_i287:
 call dust_i364
dust_i288:
 movb %al,120(%esi)
dust_i289:
 movb 132(%esi),%al
dust_i290:
 movb %al,122(%esi)
dust_i291:
 movb 133(%esi),%al
dust_i292:
 movb %al,124(%esi)
dust_i293:
 xorb $128,%al
 movb %al,%dh
dust_i294:
 call dust_i461
dust_i295:
 movb %al,133(%esi)
dust_i296:
 movb %bl,%al
dust_i297:
 movzbl %cl,%ebp
 movb %al,57(%esi,%ebp)
dust_i298:
 movb 134(%esi),%al
dust_i299:
 movb %al,122(%esi)
dust_i300:
 movb 135(%esi),%al
 movb %al,%dh
dust_i301:
 movb %al,124(%esi)
dust_i302:
 call dust_i461
dust_i303:
 movb %al,124(%esi)
dust_i304:
 movb %bl,122(%esi)
dust_i305:
 movb $0,%al
dust_i306:
 movb %al,118(%esi)
dust_i307:
 movb 142(%esi),%al
 movb %al,%dh
dust_i308:
 call dust_i560
dust_i309:
 movb 133(%esi),%al
dust_i310:
 movzbl %cl,%ebp
 movb %al,0(%esi,%ebp)
dust_i311:
 movb %al,136(%esi)
dust_i312:
 andb $127,%al
dust_i313:
 movb %al,%ah
 subb $116,%ah
 setae %dl
 andb $1,%dl
 movb %ah,%dh
dust_i314:
 testb %dl,%dl
 jne dust_i339
dust_i315:
 movb 135(%esi),%al
dust_i316:
 movzbl %cl,%ebp
 movb %al,19(%esi,%ebp)
dust_i317:
 movb %al,138(%esi)
dust_i318:
 andb $127,%al
dust_i319:
 movb %al,%ah
 subb $116,%ah
 setae %dl
 andb $1,%dl
 movb %ah,%dh
dust_i320:
 testb %dl,%dl
 jne dust_i347
dust_i321:
 call dust_i564
dust_i322:
 subb $1,%cl
 movb %cl,%dh
dust_i323:
 testb %dh,%dh
 je dust_i325
dust_i324:
 jmp dust_i251
dust_i325:
 movb 142(%esi),%al
dust_i326:
 movb 158(%esi),%ah
 xorb %ah,%al
dust_i327:
 movb %al,142(%esi)
dust_i328:
 movb 148(%esi),%al
dust_i329:
 movb 158(%esi),%ah
 xorb %ah,%al
dust_i330:
 movb %al,148(%esi)
dust_i331:
 xorb $128,%al
dust_i332:
 movb %al,149(%esi)
dust_i333:
 movb 152(%esi),%al
dust_i334:
 movb 158(%esi),%ah
 xorb %ah,%al
dust_i335:
 movb %al,152(%esi)
dust_i336:
 xorb $128,%al
 movb %al,%dh
dust_i337:
 movb %al,153(%esi)
dust_i338:
 ret
dust_i339:
 call dust_i533
dust_i340:
 movb %al,138(%esi)
dust_i341:
 movzbl %cl,%ebp
 movb %al,19(%esi,%ebp)
dust_i342:
 movb $115,%al
dust_i343:
 orb 158(%esi),%al
 movb %al,%dh
dust_i344:
 movb %al,136(%esi)
dust_i345:
 movzbl %cl,%ebp
 movb %al,0(%esi,%ebp)
dust_i346:
 testb %dh,%dh
 jne dust_i354
dust_i347:
 call dust_i533
dust_i348:
 movb %al,136(%esi)
dust_i349:
 movzbl %cl,%ebp
 movb %al,0(%esi,%ebp)
dust_i350:
 movb $110,%al
dust_i351:
 orb 149(%esi),%al
 movb %al,%dh
dust_i352:
 movb %al,138(%esi)
dust_i353:
 movzbl %cl,%ebp
 movb %al,19(%esi,%ebp)
dust_i354:
 call dust_i533
dust_i355:
 orb $8,%al
 movb %al,%dh
dust_i356:
 movb %al,140(%esi)
dust_i357:
 movzbl %cl,%ebp
 movb %al,38(%esi,%ebp)
dust_i358:
 testb %dh,%dh
 jne dust_i321
dust_i359:
 movb 132(%esi),%bl
dust_i360:
 movb %bl,122(%esi)
dust_i361:
 movb 133(%esi),%bl
dust_i362:
 movb %bl,124(%esi)
dust_i363:
 movb 146(%esi),%bl
dust_i364:
 movb %bl,118(%esi)
dust_i365:
 movb %al,%bl
dust_i366:
 andb $128,%al
dust_i367:
 movb %al,126(%esi)
dust_i368:
 movb %bl,%al
dust_i369:
 andb $127,%al
 movb %al,%dh
dust_i370:
 testb %dh,%dh
 je dust_i426
dust_i371:
 movb %al,%bl
dust_i372:
 subb $1,%bl
dust_i373:
 movb %bl,128(%esi)
dust_i374:
 movb $0,%al
dust_i375:
 movb 118(%esi),%ah
 shrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i376:
 testb %dl,%dl
 je dust_i378
dust_i377:
 movb 128(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i378:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i379:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i380:
 testb %dl,%dl
 je dust_i382
dust_i381:
 movb 128(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i382:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i383:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i384:
 testb %dl,%dl
 je dust_i386
dust_i385:
 movb 128(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i386:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i387:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i388:
 testb %dl,%dl
 je dust_i390
dust_i389:
 movb 128(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i390:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i391:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i392:
 testb %dl,%dl
 je dust_i394
dust_i393:
 movb 128(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i394:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i395:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 movb %ah,118(%esi)
dust_i396:
 shrb $1,%al
 setb %dl
 andb $1,%dl
dust_i397:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 movb %ah,118(%esi)
dust_i398:
 shrb $1,%al
 setb %dl
 andb $1,%dl
dust_i399:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 movb %ah,118(%esi)
dust_i400:
 shrb $1,%al
 setb %dl
 andb $1,%dl
dust_i401:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i402:
 orb 126(%esi),%al
 movb %al,%dh
dust_i403:
 ret
dust_i404:
 movb $0,%dl
dust_i405:
 movb %bl,118(%esi)
dust_i406:
 movb %bl,%al
 movb %al,%dh
dust_i407:
 ret
dust_i408:
 movzbl %cl,%ebp
 movb 19(%esi,%ebp),%al
dust_i409:
 movb %al,138(%esi)
dust_i410:
 andb $127,%al
dust_i411:
 movb %al,118(%esi)
dust_i412:
 movb 120(%esi),%bl
 movb %bl,%dh
dust_i413:
 testb %dh,%dh
 je dust_i404
dust_i414:
 subb $1,%bl
dust_i415:
 movb %bl,126(%esi)
dust_i416:
 movb $0,%al
dust_i417:
 movb $8,%bl
dust_i418:
 movb 118(%esi),%ah
 shrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i419:
 testb %dl,%dl
 je dust_i421
dust_i420:
 movb 126(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i421:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i422:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i423:
 subb $1,%bl
 movb %bl,%dh
dust_i424:
 testb %dh,%dh
 jne dust_i419
dust_i425:
 ret
dust_i426:
 movb %al,119(%esi)
dust_i427:
 movb %al,118(%esi)
dust_i428:
 ret
dust_i429:
 movb 133(%esi),%bl
dust_i430:
 movb %bl,124(%esi)
dust_i431:
 movb 132(%esi),%bl
dust_i432:
 movb %bl,122(%esi)
dust_i433:
 movb %al,%bl
dust_i434:
 andb $127,%al
dust_i435:
 shrb $1,%al
 setb %dl
 andb $1,%dl
dust_i436:
 movb %al,118(%esi)
dust_i437:
 movb %bl,%al
dust_i438:
 movb 120(%esi),%ah
 xorb %ah,%al
dust_i439:
 andb $128,%al
dust_i440:
 movb %al,126(%esi)
dust_i441:
 movb 120(%esi),%al
dust_i442:
 andb $127,%al
 movb %al,%dh
dust_i443:
 testb %dh,%dh
 je dust_i459
dust_i444:
 movb %al,%bl
dust_i445:
 subb $1,%bl
dust_i446:
 movb %bl,128(%esi)
dust_i447:
 movb $0,%al
dust_i448:
 movb $7,%bl
dust_i449:
 testb %dl,%dl
 je dust_i451
dust_i450:
 movb 128(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i451:
 shrb $1,%dl
 rcrb $1,%al
 setb %dl
 andb $1,%dl
dust_i452:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i453:
 subb $1,%bl
 movb %bl,%dh
dust_i454:
 testb %dh,%dh
 jne dust_i449
dust_i455:
 shrb $1,%al
 setb %dl
 andb $1,%dl
dust_i456:
 shrb $1,%dl
 movb 118(%esi),%ah
 rcrb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i457:
 orb 126(%esi),%al
 movb %al,%dh
dust_i458:
 ret
dust_i459:
 movb %al,118(%esi)
dust_i460:
 ret
dust_i461:
 call dust_i433
dust_i462:
 movb %al,128(%esi)
dust_i463:
 andb $128,%al
dust_i464:
 movb %al,126(%esi)
dust_i465:
 movb 124(%esi),%ah
 xorb %ah,%al
 movb %al,%dh
dust_i466:
 testb %dh,%dh
 js dust_i475
dust_i467:
 movb 122(%esi),%al
dust_i468:
 movb $0,%dl
dust_i469:
 movb 118(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i470:
 movb %al,%bl
dust_i471:
 movb 124(%esi),%al
dust_i472:
 movb 128(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i473:
 orb 126(%esi),%al
 movb %al,%dh
dust_i474:
 ret
dust_i475:
 movb 124(%esi),%al
dust_i476:
 andb $127,%al
dust_i477:
 movb %al,130(%esi)
dust_i478:
 movb 118(%esi),%al
dust_i479:
 movb $1,%dl
dust_i480:
 movb 122(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
 setae %dl
 andb $1,%dl
dust_i481:
 movb %al,%bl
dust_i482:
 movb 128(%esi),%al
dust_i483:
 andb $127,%al
dust_i484:
 movb 130(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
 setae %dl
 andb $1,%dl
dust_i485:
 testb %dl,%dl
 jne dust_i494
dust_i486:
 movb %al,130(%esi)
dust_i487:
 movb %bl,%al
dust_i488:
 xorb $255,%al
dust_i489:
 movb $1,%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i490:
 movb %al,%bl
dust_i491:
 movb $0,%al
dust_i492:
 movb 130(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
 setae %dl
 andb $1,%dl
dust_i493:
 orb $128,%al
dust_i494:
 movb 126(%esi),%ah
 xorb %ah,%al
 movb %al,%dh
dust_i495:
 ret
dust_i496:
 movzbl %cl,%ebp
 movb 38(%esi,%ebp),%al
dust_i497:
 movb %al,120(%esi)
dust_i498:
 movb 154(%esi),%al
dust_i499:
 movb $8,%bl
dust_i500:
 shlb $1,%al
 setb %dl
 andb $1,%dl
dust_i501:
 movb %al,118(%esi)
dust_i502:
 movb $0,%al
dust_i503:
 shrb $1,%dl
 rclb $1,%al
 setb %dl
 andb $1,%dl
dust_i504:
 testb %dl,%dl
 jne dust_i507
dust_i505:
 movb %al,%ah
 subb 120(%esi),%ah
 setae %dl
 andb $1,%dl
dust_i506:
 testb %dl,%dl
 je dust_i509
dust_i507:
 movb 120(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
dust_i508:
 movb $1,%dl
dust_i509:
 shrb $1,%dl
 movb 118(%esi),%ah
 rclb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,118(%esi)
dust_i510:
 subb $1,%bl
 movb %bl,%dh
dust_i511:
 testb %dh,%dh
 jne dust_i503
dust_i512:
 jmp dust_i515
dust_i513:
 movb %al,%ah
 subb 120(%esi),%ah
 setae %dl
 andb $1,%dl
dust_i514:
 testb %dl,%dl
 jne dust_i530
dust_i515:
 movb $254,%bl
dust_i516:
 movb %bl,122(%esi)
dust_i517:
 shlb $1,%al
 setb %dl
 andb $1,%dl
dust_i518:
 testb %dl,%dl
 jne dust_i525
dust_i519:
 movb %al,%ah
 subb 120(%esi),%ah
 setae %dl
 andb $1,%dl
dust_i520:
 testb %dl,%dl
 je dust_i522
dust_i521:
 movb 120(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
 setae %dl
 andb $1,%dl
dust_i522:
 shrb $1,%dl
 movb 122(%esi),%ah
 rclb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,122(%esi)
 movb %ah,%dh
dust_i523:
 testb %dl,%dl
 jne dust_i517
dust_i524:
 ret
dust_i525:
 movb 120(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
dust_i526:
 movb $1,%dl
dust_i527:
 shrb $1,%dl
 movb 122(%esi),%ah
 rclb $1,%ah
 setb %dl
 andb $1,%dl
 movb %ah,122(%esi)
 movb %ah,%dh
dust_i528:
 testb %dl,%dl
 jne dust_i517
dust_i529:
 ret
dust_i530:
 movb $255,%al
 movb %al,%dh
dust_i531:
 movb %al,122(%esi)
dust_i532:
 ret
dust_i533:
 movb 114(%esi),%al
dust_i534:
 shrb $1,%dl
 rclb $1,%al
 setb %dl
 andb $1,%dl
dust_i535:
 movb %al,%bl
dust_i536:
 movb 116(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i537:
 movb %al,114(%esi)
dust_i538:
 movb %bl,116(%esi)
dust_i539:
 movb 115(%esi),%al
dust_i540:
 movb %al,%bl
dust_i541:
 movb 117(%esi),%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
 movb %al,%dh
dust_i542:
 movb %al,115(%esi)
dust_i543:
 movb %bl,117(%esi)
dust_i544:
 ret
dust_i545:
 movb $10,%cl
 movb %cl,%dh
dust_i546:
 call dust_i533
dust_i547:
 orb $8,%al
 movb %al,%dh
dust_i548:
 movzbl %cl,%ebp
 movb %al,38(%esi,%ebp)
dust_i549:
 movb %al,140(%esi)
dust_i550:
 call dust_i533
dust_i551:
 movzbl %cl,%ebp
 movb %al,0(%esi,%ebp)
dust_i552:
 movb %al,136(%esi)
dust_i553:
 call dust_i533
dust_i554:
 movzbl %cl,%ebp
 movb %al,19(%esi,%ebp)
dust_i555:
 movb %al,138(%esi)
dust_i556:
 call dust_i564
dust_i557:
 subb $1,%cl
 movb %cl,%dh
dust_i558:
 testb %dh,%dh
 jne dust_i546
dust_i559:
 ret
dust_i560:
 call dust_i462
dust_i561:
 movb %al,135(%esi)
dust_i562:
 movb %bl,%al
dust_i563:
 movzbl %cl,%ebp
 movb %al,76(%esi,%ebp)
dust_i564:
 movb 136(%esi),%al
 movb %al,%dh
dust_i565:
 testb %dh,%dh
 jns dust_i569
dust_i566:
 xorb $127,%al
dust_i567:
 movb $0,%dl
dust_i568:
 movb $1,%ah
 shrb $1,%dl
 adcb %ah,%al
dust_i569:
 xorb $128,%al
dust_i570:
 movb %al,%bl
dust_i571:
 movb 138(%esi),%al
dust_i572:
 andb $127,%al
dust_i573:
 movb %al,%ah
 subb $96,%ah
 setae %dl
 andb $1,%dl
 movb %ah,%dh
dust_i574:
 testb %dl,%dl
 jne dust_i584
dust_i575:
 movb 138(%esi),%al
 movb %al,%dh
dust_i576:
 testb %dh,%dh
 jns dust_i579
dust_i577:
 xorb $127,%al
dust_i578:
 movb $1,%ah
 shrb $1,%dl
 adcb %ah,%al
 setb %dl
 andb $1,%dl
dust_i579:
 movb %al,126(%esi)
dust_i580:
 movb $97,%al
dust_i581:
 movb 126(%esi),%ah
 xorb $1,%dl
 shrb $1,%dl
 sbbb %ah,%al
dust_i582:
 call dust_plot
dust_i583:
 ret
dust_i584:
 ret
dust_i585:
 movb $10,%cl
dust_i586:
 movzbl %cl,%ebp
 movb 0(%esi,%ebp),%al
dust_i587:
 movb %al,136(%esi)
dust_i588:
 movzbl %cl,%ebp
 movb 19(%esi,%ebp),%al
dust_i589:
 movb %al,138(%esi)
dust_i590:
 movzbl %cl,%ebp
 movb 38(%esi,%ebp),%al
 movb %al,%dh
dust_i591:
 movb %al,140(%esi)
dust_i592:
 call dust_i564
dust_i593:
 subb $1,%cl
 movb %cl,%dh
dust_i594:
 testb %dh,%dh
 jne dust_i586
dust_i595:
 ret
