

void Delay_s(long x)
{
 for(int i = 0; i < x*1000; ++i)
 {
   __watchdog_reset();
   delay_ms(1);
 }
}

void Delay_ms(long x)
{
  for(int i = 0; i < x; ++i)
 {
   __watchdog_reset();
   delay_ms(1);
 } 
}
