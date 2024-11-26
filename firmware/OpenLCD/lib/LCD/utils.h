// Prevent display of -0 values by snapping to positive zero
// \a_number original number
// \a_precisionCount number of digits of decimal precision eg. 2 for #.##, 0 for whole integer. Default 0 (whole integer number.)
// \returns number rounded to positive zero if result would have produced -0.00 for precision.
// Taken from: https://stackoverflow.com/questions/6605147/get-printf-to-ignore-the-negative-sign-on-values-of-zero
#ifndef LCD_UTILS_H

#define LCD_UTILS_H
#include <avr/wdt.h> //Watchdog to prevent system freeze
template <class Real>
Real PosZero(const Real& a_number, const int a_precisionCount = 0)
{
    Real precisionValue = Real(0.5) * pow(Real(0.10), Real(a_precisionCount));
    if( (a_number > -abs(precisionValue)) && (a_number < abs(precisionValue)) )
    {
        return +0.0;
    }
    return a_number;
}


// Concacenating character arrays
// Taken from: https://stackoverflow.com/questions/24255051/concatenate-char-arrays-in-c
template <typename Result>
void concatenate(Result *res)
{
  return;
}

template <typename Result, typename T>
void concatenate(Result *res, T *str)
{
  strcat(res, str);
}

template <typename Result, typename First, typename ... T>
void concatenate(Result *res, First *f, T* ... next)
{
  strcat(res, f);
  concatenate(res, next...);
}

template <typename Result, typename First, typename ... T>
void concatStrings(Result *res, First *f, T* ... next)
{
  strcpy(res, f);
  concatenate(res, next...);
}

// Delay
//Delays for a specified period that is pet safe
void petSafeDelay(int delayAmount)
{
  long startTime = millis();

  while ((unsigned long)(millis() - startTime) <= delayAmount)
  {
    wdt_reset(); //Pet the dog

    //Max 100ms delay
    for (byte x = 0 ; x < 100 ; x++)
    {
      delay(1);
      if ( (unsigned long)(millis() - startTime) >= delayAmount) break;
    }
  }

  wdt_reset(); //Pet the dog
}

#endif