/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <inc/time.h>
#include <inc/vsyscall.h>
#include <kern/vsyscall.h>

int gettime(void)
{
	nmi_disable();
	// LAB 12: your code here

	struct tm cur_time;

	cur_time.tm_sec = BCD2BIN(mc146818_read(RTC_SEC));
	cur_time.tm_min = BCD2BIN(mc146818_read(RTC_MIN));
	cur_time.tm_hour = BCD2BIN(mc146818_read(RTC_HOUR));
	cur_time.tm_mday = BCD2BIN(mc146818_read(RTC_DAY));
	cur_time.tm_mon = BCD2BIN(mc146818_read(RTC_MON))-1;
	cur_time.tm_year= BCD2BIN(mc146818_read(RTC_YEAR));

	int time = timestamp(&cur_time);
	vsys[VSYS_gettime] = time;
	nmi_enable();

	return time;
}

void
rtc_init(void)
{
	nmi_disable();
	// LAB 4: your code here
	//Для управления часами используются три байта в памяти CMOS, называемые регистрами A, B и C. 
	//Для работы с этими регистрами используются порты ввода-вывода 0x70 и 0x71 
	//(в JOS для работы с ними используются константы IO_RTC_CMND и IO_RTC_DATA).
	outb(IO_RTC_CMND, RTC_BREG);//1. Переключение на регистр часов B.
	uint8_t regB = inb(IO_RTC_DATA);//2. Чтение значения регистра B из порта ввода-вывода.
	regB = regB | RTC_PIE;//3. Установка бита RTC_PIE.
	outb(IO_RTC_CMND, RTC_BREG);//4. Запись обновленного значения регистра в порт ввода-вывода.
	outb(IO_RTC_DATA, regB);
	//меняем частоту часов, чтобы прерывания приходили раз в 0.5 сек
	outb(IO_RTC_CMND, RTC_AREG);
	uint8_t regA = inb(IO_RTC_DATA);
	regA = regA & 0xF0;
	regA = regA | 0x0F;
	outb(IO_RTC_CMND, RTC_AREG);
	outb(IO_RTC_DATA, regA);
	nmi_enable();
}

uint8_t
rtc_check_status(void)
{
	uint8_t status = 0;
	// LAB 4: your code here
	outb(IO_RTC_CMND, RTC_CREG);
	status = inb(IO_RTC_DATA);
	return status;
}

unsigned
mc146818_read(unsigned reg)
{
	outb(IO_RTC_CMND, reg);
	return inb(IO_RTC_DATA);
}

void
mc146818_write(unsigned reg, unsigned datum)
{
	outb(IO_RTC_CMND, reg);
	outb(IO_RTC_DATA, datum);
}

