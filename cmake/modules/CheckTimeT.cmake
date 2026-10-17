include(CheckTypeSize)
include(CheckCSourceCompiles)

function(flvmeta_check_time_t)
  set(CMAKE_EXTRA_INCLUDE_FILES time.h)
  check_type_size("time_t" SIZEOF_TIME_T)
  check_c_source_compiles([[
    #include <time.h>
    typedef char signed_time_t[((time_t)-1 < (time_t)0) ? 1 : -1];
    int main(void) { return 0; }
  ]] TIME_T_IS_SIGNED)

  if(SIZEOF_TIME_T EQUAL 4)
    if(TIME_T_IS_SIGNED)
      set(TIME_T_MIN "(-2147483647L - 1)")
      set(TIME_T_MAX "2147483647L")
    else()
      set(TIME_T_MIN "0UL")
      set(TIME_T_MAX "4294967295UL")
    endif()
  elseif(SIZEOF_TIME_T EQUAL 8)
    if(TIME_T_IS_SIGNED)
      set(TIME_T_MIN "(-9223372036854775807LL - 1)")
      set(TIME_T_MAX "9223372036854775807LL")
    else()
      set(TIME_T_MIN "0ULL")
      set(TIME_T_MAX "18446744073709551615ULL")
    endif()
  else()
    message(FATAL_ERROR "Unsupported time_t size: ${SIZEOF_TIME_T}")
  endif()

  # Confirm that time_t is an integer and has the selected bounds.
  check_c_source_compiles("
    #include <limits.h>
    #include <time.h>
    typedef char integer_time_t[1 + (time_t)0 % (time_t)1];
    typedef char time_t_bounds[(CHAR_BIT == 8 &&
      (time_t)(${TIME_T_MIN}) == (${TIME_T_MIN}) &&
      (time_t)(${TIME_T_MAX}) == (${TIME_T_MAX})) ? 1 : -1];
    int main(void) { return 0; }
  " TIME_T_BOUNDS_VALID)
  if(NOT TIME_T_BOUNDS_VALID)
    message(FATAL_ERROR "Unsupported time_t: expected a signed or unsigned 32-bit or 64-bit integer")
  endif()

  set(SIZEOF_TIME_T "${SIZEOF_TIME_T}" PARENT_SCOPE)
  set(TIME_T_IS_SIGNED "${TIME_T_IS_SIGNED}" PARENT_SCOPE)
  set(TIME_T_MIN "${TIME_T_MIN}" PARENT_SCOPE)
  set(TIME_T_MAX "${TIME_T_MAX}" PARENT_SCOPE)
endfunction()
