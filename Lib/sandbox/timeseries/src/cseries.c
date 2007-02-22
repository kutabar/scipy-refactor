#include <Python.h>
#include <structmember.h>
#include <stdio.h>
#include <string.h>
#include "mxDateTime.h"
#include "arrayobject.h"

static char cseries_doc[] = "Speed sensitive time series operations";

#define FR_ANN  1000  /* Annual */
#define FR_QTR  2000  /* Quarterly */
#define FR_MTH  3000  /* Monthly */
#define FR_WK   4000  /* Weekly */
#define FR_BUS  5000  /* Business days */
#define FR_DAY   6000  /* Daily */
#define FR_HR   7000  /* Hourly */
#define FR_MIN  8000  /* Minutely */
#define FR_SEC  9000  /* Secondly */
#define FR_UND  -9999 /* Undefined */

#define ADD_INT_TO_DICT(dict, key, val) \
    {PyObject *pyval = PyInt_FromLong(val); \
     PyDict_SetItemString(dict, key, pyval); \
     Py_DECREF(pyval); }

static long minval_D_toHighFreq = 719163;

///////////////////////////////////////////////////////////////////////

// helpers for frequency conversion routines //

static long DtoB_weekday(long fromDate) { return (((fromDate) / 7) * 5) + (fromDate)%7; }

static long DtoB_WeekendToMonday(mxDateTimeObject *dailyDate) {

    long absdate = dailyDate->absdate;
    if (dailyDate->day_of_week > 4) {
        //change to Monday after weekend
        absdate += (7 - dailyDate->day_of_week);
    }
    return DtoB_weekday(absdate);
}

static long DtoB_WeekendToFriday(mxDateTimeObject *dailyDate) {

    long absdate = dailyDate->absdate;
    if (dailyDate->day_of_week > 4) {
        //change to friday before weekend
        absdate -= (dailyDate->day_of_week - 4);
    }
    return DtoB_weekday(absdate);
}

static long absdate_from_ymd(int y, int m, int d) {
    mxDateTimeObject *tempDate;
    long result;

    tempDate = (mxDateTimeObject *)mxDateTime.DateTime_FromDateAndTime(y,m,d,0,0,0);
    result = (long)(tempDate->absdate);
    Py_DECREF(tempDate);
    return result;
}


static mxDateTimeObject *day_before_mxobj(int y, int m, int d) {
    return  (mxDateTimeObject *)mxDateTime.DateTime_FromAbsDateAndTime(absdate_from_ymd(y, m, d) - 1, 0);
}

/*
returns Date(y, m, d) converted to business frequency.
If the initial result is a weekend, the following Monday is returned
*/
static long busday_before(int y, int m, int d) {
    mxDateTimeObject *dailyDate;
    long result;

    dailyDate = (mxDateTimeObject *)mxDateTime.DateTime_FromDateAndTime(y,m,d,0,0,0);
    result = DtoB_WeekendToMonday(dailyDate);

    Py_DECREF(dailyDate);
    return result;
}

/*
returns Date(y, m, d) - 1 converted to business frequency.
If the initial result is a weekend, the preceding Friday is returned
*/
static long busday_after(int y, int m, int d) {
    mxDateTimeObject *dailyDate;
    long result;

    dailyDate = day_before_mxobj(y,m,d);
    result = DtoB_WeekendToFriday(dailyDate);

    Py_DECREF(dailyDate);
    return result;
}

///////////////////////////////////////////////

// frequency specifc conversion routines
// each function must take an integer fromDate and a char relation ('B' or 'A' for 'BEFORE' or 'AFTER')

//************ FROM DAILY ***************

static long asfreq_DtoA(long fromDate, char relation) {
    mxDateTimeObject *mxDate;
    long result;

    mxDate = (mxDateTimeObject *)mxDateTime.DateTime_FromAbsDateAndTime(fromDate, 0);
    result = (long)(mxDate->year);
    Py_DECREF(mxDate);
    return result;
}

static long asfreq_DtoQ(long fromDate, char relation) {

    mxDateTimeObject *mxDate;
    long result;

    mxDate = (mxDateTimeObject *)mxDateTime.DateTime_FromAbsDateAndTime(fromDate, 0);
    result = (long)((mxDate->year - 1) * 4 + (mxDate->month-1)/3 + 1);
    Py_DECREF(mxDate);
    return result;
}

static long asfreq_DtoM(long fromDate, char relation) {
    mxDateTimeObject *mxDate;
    long result;

    mxDate = (mxDateTimeObject *)mxDateTime.DateTime_FromAbsDateAndTime(fromDate, 0);
    result = (long)((mxDate->year - 1) * 12 + mxDate->month);
    Py_DECREF(mxDate);
    return result;
}

static long asfreq_DtoW(long fromDate, char relation) { return (fromDate - 1)/7 + 1; }

static long asfreq_DtoB(long fromDate, char relation) {
    mxDateTimeObject *mxDate;
    long result;

    mxDate = (mxDateTimeObject *)mxDateTime.DateTime_FromAbsDateAndTime(fromDate, 0);
    if (relation == 'B') {
        result = DtoB_WeekendToFriday(mxDate);
    } else {
        result = DtoB_WeekendToMonday(mxDate);
    }

    Py_DECREF(mxDate);
    return result;
}

static long asfreq_DtoB_forConvert(long fromDate, char relation) {
    mxDateTimeObject *mxDate;
    long result;

    mxDate = (mxDateTimeObject *)mxDateTime.DateTime_FromAbsDateAndTime(fromDate, 0);

    if (mxDate->day_of_week > 4) {
        result = -1;
    } else {
        result = DtoB_weekday(mxDate->absdate);
    }

    Py_DECREF(mxDate);
    return result;
}

// needed for getDateInfo function
static long asfreq_DtoD(long fromDate, char relation) { return fromDate; }

static long asfreq_DtoHIGHFREQ(long fromDate, char relation, long periodsPerDay) {
    if (fromDate >= minval_D_toHighFreq) {
        if (relation == 'B') { return (fromDate - minval_D_toHighFreq)*(periodsPerDay) + 1; }
        else                 { return (fromDate - minval_D_toHighFreq + 1)*(periodsPerDay); }
    } else { return -1; }
}

static long asfreq_DtoH(long fromDate, char relation) { return asfreq_DtoHIGHFREQ(fromDate, relation, 24); }
static long asfreq_DtoT(long fromDate, char relation) { return asfreq_DtoHIGHFREQ(fromDate, relation, 24*60); }
static long asfreq_DtoS(long fromDate, char relation) { return asfreq_DtoHIGHFREQ(fromDate, relation, 24*60*60); }

//************ FROM SECONDLY ***************

static long asfreq_StoD(long fromDate, char relation) { return (fromDate - 1)/(60*60*24) + minval_D_toHighFreq; }

static long asfreq_StoA(long fromDate, char relation) { return asfreq_DtoA(asfreq_StoD(fromDate, relation), relation); }
static long asfreq_StoQ(long fromDate, char relation) { return asfreq_DtoQ(asfreq_StoD(fromDate, relation), relation); }
static long asfreq_StoM(long fromDate, char relation) { return asfreq_DtoM(asfreq_StoD(fromDate, relation), relation); }
static long asfreq_StoW(long fromDate, char relation) { return asfreq_DtoW(asfreq_StoD(fromDate, relation), relation); }
static long asfreq_StoB(long fromDate, char relation) { return asfreq_DtoB(asfreq_StoD(fromDate, relation), relation); }
static long asfreq_StoB_forConvert(long fromDate, char relation)
    { return asfreq_DtoB_forConvert(asfreq_StoD(fromDate, relation), relation); }
static long asfreq_StoT(long fromDate, char relation) { return (fromDate - 1)/60 + 1; }
static long asfreq_StoH(long fromDate, char relation) { return (fromDate - 1)/(60*60) + 1; }

//************ FROM MINUTELY ***************

static long asfreq_TtoD(long fromDate, char relation) { return (fromDate - 1)/(60*24) + minval_D_toHighFreq; }

static long asfreq_TtoA(long fromDate, char relation) { return asfreq_DtoA(asfreq_TtoD(fromDate, relation), relation); }
static long asfreq_TtoQ(long fromDate, char relation) { return asfreq_DtoQ(asfreq_TtoD(fromDate, relation), relation); }
static long asfreq_TtoM(long fromDate, char relation) { return asfreq_DtoM(asfreq_TtoD(fromDate, relation), relation); }
static long asfreq_TtoW(long fromDate, char relation) { return asfreq_DtoW(asfreq_TtoD(fromDate, relation), relation); }
static long asfreq_TtoB(long fromDate, char relation) { return asfreq_DtoB(asfreq_TtoD(fromDate, relation), relation); }

static long asfreq_TtoB_forConvert(long fromDate, char relation)
    { return asfreq_DtoB_forConvert(asfreq_TtoD(fromDate, relation), relation); }

static long asfreq_TtoH(long fromDate, char relation) { return (fromDate - 1)/60 + 1; }
static long asfreq_TtoS(long fromDate, char relation) {
    if (relation == 'B') {  return fromDate*60 - 59; }
    else                 {  return fromDate*60;      }}

//************ FROM HOURLY ***************

static long asfreq_HtoD(long fromDate, char relation) { return (fromDate - 1)/24 + minval_D_toHighFreq; }
static long asfreq_HtoA(long fromDate, char relation) { return asfreq_DtoA(asfreq_HtoD(fromDate, relation), relation); }
static long asfreq_HtoQ(long fromDate, char relation) { return asfreq_DtoQ(asfreq_HtoD(fromDate, relation), relation); }
static long asfreq_HtoM(long fromDate, char relation) { return asfreq_DtoM(asfreq_HtoD(fromDate, relation), relation); }
static long asfreq_HtoW(long fromDate, char relation) { return asfreq_DtoW(asfreq_HtoD(fromDate, relation), relation); }
static long asfreq_HtoB(long fromDate, char relation) { return asfreq_DtoB(asfreq_HtoD(fromDate, relation), relation); }

static long asfreq_HtoB_forConvert(long fromDate, char relation)
    { return asfreq_DtoB_forConvert(asfreq_HtoD(fromDate, relation), relation); }

// calculation works out the same as TtoS, so we just call that function for HtoT
static long asfreq_HtoT(long fromDate, char relation) { return asfreq_TtoS(fromDate, relation); }
static long asfreq_HtoS(long fromDate, char relation) {
    if (relation == 'B') {  return fromDate*60*60 - 60*60 + 1; }
    else                 {  return fromDate*60*60;             }}

//************ FROM BUSINESS ***************

static long asfreq_BtoD(long fromDate, char relation) {
    return ((fromDate-1)/5)*7 + (fromDate-1)%5 + 1; }

static long asfreq_BtoA(long fromDate, char relation) {
    return asfreq_DtoA(asfreq_BtoD(fromDate, relation), relation); }

static long asfreq_BtoQ(long fromDate, char relation) {
    return asfreq_DtoQ(asfreq_BtoD(fromDate, relation), relation); }

static long asfreq_BtoM(long fromDate, char relation) {
    return asfreq_DtoM(asfreq_BtoD(fromDate, relation), relation); }

static long asfreq_BtoW(long fromDate, char relation) {
    return asfreq_DtoW(asfreq_BtoD(fromDate, relation), relation); }

static long asfreq_BtoH(long fromDate, char relation) {
    return asfreq_DtoH(asfreq_BtoD(fromDate, relation), relation); }

static long asfreq_BtoT(long fromDate, char relation) {
    return asfreq_DtoT(asfreq_BtoD(fromDate, relation), relation); }

static long asfreq_BtoS(long fromDate, char relation) {
    return asfreq_DtoS(asfreq_BtoD(fromDate, relation), relation); }

//************ FROM WEEKLY ***************

static long asfreq_WtoD(long fromDate, char relation) {
    if (relation == 'B') { return fromDate * 7 - 6;}
    else                 { return fromDate * 7; }}

static long asfreq_WtoA(long fromDate, char relation) {
    return asfreq_DtoA(asfreq_WtoD(fromDate, 'A'), relation); }
static long asfreq_WtoQ(long fromDate, char relation) {
    return asfreq_DtoQ(asfreq_WtoD(fromDate, 'A'), relation); }
static long asfreq_WtoM(long fromDate, char relation) {
    return asfreq_DtoM(asfreq_WtoD(fromDate, 'A'), relation); }

static long asfreq_WtoB(long fromDate, char relation) {
    mxDateTimeObject *mxDate;
    long result;

    mxDate = (mxDateTimeObject *)mxDateTime.DateTime_FromAbsDateAndTime(asfreq_WtoD(fromDate, relation), 0);
    if (relation == 'B') { result = DtoB_WeekendToMonday(mxDate); }
    else                 { result = DtoB_WeekendToFriday(mxDate); }
    Py_DECREF(mxDate);
    return result;
}

static long asfreq_WtoH(long fromDate, char relation) {
    return asfreq_DtoH(asfreq_WtoD(fromDate, relation), relation); }
static long asfreq_WtoT(long fromDate, char relation) {
    return asfreq_DtoT(asfreq_WtoD(fromDate, relation), relation); }
static long asfreq_WtoS(long fromDate, char relation) {
    return asfreq_DtoS(asfreq_WtoD(fromDate, relation), relation); }

//************ FROM MONTHLY ***************

static void MtoD_ym(long fromDate, long *y, long *m) {
    *y = (fromDate - 1) / 12 + 1;
    *m = fromDate - 12 * (*y) - 1;
}

static long asfreq_MtoD(long fromDate, char relation) {

    long y, m;

    if (relation == 'B') {
        MtoD_ym(fromDate, &y, &m);
        return absdate_from_ymd(y, m, 1);
    } else {
        MtoD_ym(fromDate+1, &y, &m);
        return absdate_from_ymd(y, m, 1) - 1;
    }
}

static long asfreq_MtoA(long fromDate, char relation) { return (fromDate - 1) / 12 + 1; }
static long asfreq_MtoQ(long fromDate, char relation) { return (fromDate - 1) / 3 + 1; }

static long asfreq_MtoW(long fromDate, char relation) { return asfreq_DtoW(asfreq_MtoD(fromDate, relation), relation); }

static long asfreq_MtoB(long fromDate, char relation) {

    mxDateTimeObject *mxDate;
    long result;

    mxDate = (mxDateTimeObject *)mxDateTime.DateTime_FromAbsDateAndTime(asfreq_MtoD(fromDate, relation), 0);
    if (relation == 'B') { result = DtoB_WeekendToMonday(mxDate); }
    else                 { result = DtoB_WeekendToFriday(mxDate); }
    Py_DECREF(mxDate);
    return result;
}

static long asfreq_MtoH(long fromDate, char relation) { return asfreq_DtoH(asfreq_MtoD(fromDate, relation), relation); }
static long asfreq_MtoT(long fromDate, char relation) { return asfreq_DtoT(asfreq_MtoD(fromDate, relation), relation); }
static long asfreq_MtoS(long fromDate, char relation) { return asfreq_DtoS(asfreq_MtoD(fromDate, relation), relation); }

//************ FROM QUARTERLY ***************

static void QtoD_ym(long fromDate, long *y, long *m) {
    *y = (fromDate - 1) / 4 + 1;
    *m = (fromDate + 4) * 3 - 12 * (*y) - 2;
}

static long asfreq_QtoD(long fromDate, char relation) {

    long y, m;

    if (relation == 'B') {
        QtoD_ym(fromDate, &y, &m);
        return absdate_from_ymd(y, m, 1);
    } else {
        QtoD_ym(fromDate+1, &y, &m);
        return absdate_from_ymd(y, m, 1) - 1;
    }
}

static long asfreq_QtoA(long fromDate, char relation) { return (fromDate - 1)/ 4 + 1; }

static long asfreq_QtoM(long fromDate, char relation) {
    if (relation == 'B') { return fromDate * 3 - 2; }
    else {                 return fromDate  * 3; }
}

static long asfreq_QtoW(long fromDate, char relation) { return asfreq_DtoW(asfreq_QtoD(fromDate, relation), relation); }

static long asfreq_QtoB(long fromDate, char relation) {

    mxDateTimeObject *mxDate;
    long result;

    mxDate = (mxDateTimeObject *)mxDateTime.DateTime_FromAbsDateAndTime(asfreq_QtoD(fromDate, relation), 0);
    if (relation == 'B') { result = DtoB_WeekendToMonday(mxDate); }
    else                 { result = DtoB_WeekendToFriday(mxDate); }
    Py_DECREF(mxDate);
    return result;
}


static long asfreq_QtoH(long fromDate, char relation) { return asfreq_DtoH(asfreq_QtoD(fromDate, relation), relation); }
static long asfreq_QtoT(long fromDate, char relation) { return asfreq_DtoT(asfreq_QtoD(fromDate, relation), relation); }
static long asfreq_QtoS(long fromDate, char relation) { return asfreq_DtoS(asfreq_QtoD(fromDate, relation), relation); }


//************ FROM ANNUAL ***************

static long asfreq_AtoD(long fromDate, char relation) {
    if (relation == 'B') { return absdate_from_ymd(fromDate,1,1); }
    else {                 return absdate_from_ymd(fromDate+1,1,1) - 1; }
}

static long asfreq_AtoQ(long fromDate, char relation) {
    if (relation == 'B') { return fromDate * 4 - 3; }
    else {                 return fromDate * 4; }
}

static long asfreq_AtoM(long fromDate, char relation) {
    if (relation == 'B') { return fromDate * 12 - 11; }
    else {                 return fromDate * 12; }
}

static long asfreq_AtoW(long fromDate, char relation) { return asfreq_DtoW(asfreq_AtoD(fromDate, relation), relation); }

static long asfreq_AtoB(long fromDate, char relation) {
    if (relation == 'B') { return busday_before(fromDate,1,1); }
    else {                 return busday_after(fromDate+1,1,1); }
}

static long asfreq_AtoH(long fromDate, char relation) { return asfreq_DtoH(asfreq_AtoD(fromDate, relation), relation); }
static long asfreq_AtoT(long fromDate, char relation) { return asfreq_DtoT(asfreq_AtoD(fromDate, relation), relation); }
static long asfreq_AtoS(long fromDate, char relation) { return asfreq_DtoS(asfreq_AtoD(fromDate, relation), relation); }


static long nofunc(long fromDate, char relation) { return -1; }

// end of frequency specific conversion routines

// return a pointer to appropriate conversion function
static long (*get_asfreq_func(int fromFreq, int toFreq, int forConvert))(long, char) {

    switch(fromFreq)
    {
        case FR_ANN:
            switch(toFreq)
            {
                case FR_QTR: return &asfreq_AtoQ;
                case FR_MTH: return &asfreq_AtoM;
                case FR_WK: return &asfreq_AtoW;
                case FR_BUS: return &asfreq_AtoB;
                case FR_DAY: return &asfreq_AtoD;
                case FR_HR: return &asfreq_AtoH;
                case FR_MIN: return &asfreq_AtoT;
                case FR_SEC: return &asfreq_AtoS;
                default: return &nofunc;
            }

        case FR_QTR:
            switch(toFreq)
            {
                case FR_ANN: return &asfreq_QtoA;
                case FR_MTH: return &asfreq_QtoM;
                case FR_WK: return &asfreq_QtoW;
                case FR_BUS: return &asfreq_QtoB;
                case FR_DAY: return &asfreq_QtoD;
                case FR_HR: return &asfreq_QtoH;
                case FR_MIN: return &asfreq_QtoT;
                case FR_SEC: return &asfreq_QtoS;
                default: return &nofunc;
            }

        case FR_MTH:
            switch(toFreq)
            {
                case FR_ANN: return &asfreq_MtoA;
                case FR_QTR: return &asfreq_MtoQ;
                case FR_WK: return &asfreq_MtoW;
                case FR_BUS: return &asfreq_MtoB;
                case FR_DAY: return &asfreq_MtoD;
                case FR_HR: return &asfreq_MtoH;
                case FR_MIN: return &asfreq_MtoT;
                case FR_SEC: return &asfreq_MtoS;
                default: return &nofunc;
            }

        case FR_WK:
            switch(toFreq)
            {
                case FR_ANN: return &asfreq_WtoA;
                case FR_QTR: return &asfreq_WtoQ;
                case FR_MTH: return &asfreq_WtoM;
                case FR_BUS: return &asfreq_WtoB;
                case FR_DAY: return &asfreq_WtoD;
                case FR_HR: return &asfreq_WtoH;
                case FR_MIN: return &asfreq_WtoT;
                case FR_SEC: return &asfreq_WtoS;
                default: return &nofunc;
            }

        case FR_BUS:
            switch(toFreq)
            {
                case FR_ANN: return &asfreq_BtoA;
                case FR_QTR: return &asfreq_BtoQ;
                case FR_MTH: return &asfreq_BtoM;
                case FR_WK: return &asfreq_BtoW;
                case FR_DAY: return &asfreq_BtoD;
                case FR_HR: return &asfreq_BtoH;
                case FR_MIN: return &asfreq_BtoT;
                case FR_SEC: return &asfreq_BtoS;
                default: return &nofunc;
            }

        case FR_DAY:
            switch(toFreq)
            {
                case FR_ANN: return &asfreq_DtoA;
                case FR_QTR: return &asfreq_DtoQ;
                case FR_MTH: return &asfreq_DtoM;
                case FR_WK: return &asfreq_DtoW;
                case FR_BUS:
                    if (forConvert) { return &asfreq_DtoB_forConvert; }
                    else            { return &asfreq_DtoB; }
                case FR_DAY: return &asfreq_DtoD;
                case FR_HR: return &asfreq_DtoH;
                case FR_MIN: return &asfreq_DtoT;
                case FR_SEC: return &asfreq_DtoS;
                default: return &nofunc;
            }

        case FR_HR:
            switch(toFreq)
            {
                case FR_ANN: return &asfreq_HtoA;
                case FR_QTR: return &asfreq_HtoQ;
                case FR_MTH: return &asfreq_HtoM;
                case FR_WK: return &asfreq_HtoW;
                case FR_BUS:
                    if (forConvert) { return &asfreq_HtoB_forConvert; }
                    else            { return &asfreq_HtoB; }
                case FR_DAY: return &asfreq_HtoD;
                case FR_MIN: return &asfreq_HtoT;
                case FR_SEC: return &asfreq_HtoS;
                default: return &nofunc;
            }

        case FR_MIN:
            switch(toFreq)
            {
                case FR_ANN: return &asfreq_TtoA;
                case FR_QTR: return &asfreq_TtoQ;
                case FR_MTH: return &asfreq_TtoM;
                case FR_WK: return &asfreq_TtoW;
                case FR_BUS:
                    if (forConvert) { return &asfreq_TtoB_forConvert; }
                    else            { return &asfreq_TtoB; }
                case FR_DAY: return &asfreq_TtoD;
                case FR_HR: return &asfreq_TtoH;
                case FR_SEC: return &asfreq_TtoS;
                default: return &nofunc;
            }

        case FR_SEC:
            switch(toFreq)
            {
                case FR_ANN: return &asfreq_StoA;
                case FR_QTR: return &asfreq_StoQ;
                case FR_MTH: return &asfreq_StoM;
                case FR_WK: return &asfreq_StoW;
                case FR_BUS:
                    if (forConvert) { return &asfreq_StoB_forConvert; }
                    else            { return &asfreq_StoB; }
                case FR_DAY: return &asfreq_StoD;
                case FR_HR: return &asfreq_StoH;
                case FR_MIN: return &asfreq_StoT;
                default: return &nofunc;
            }
        default: return &nofunc;
    }
}

/*
Helper function for cseries_convert:
    determine the size of the second dimension for the resulting
    converted array
*/
static long get_height(int fromFreq, int toFreq) {

    int maxBusDaysPerYear, maxBusDaysPerQuarter, maxBusDaysPerMonth;
    int maxDaysPerYear, maxDaysPerQuarter, maxDaysPerMonth;

    maxBusDaysPerYear = 262;
    maxBusDaysPerQuarter = 66;
    maxBusDaysPerMonth = 23;

    maxDaysPerYear = 366;
    maxDaysPerQuarter = 92;
    maxDaysPerMonth = 31;

    switch(fromFreq)
    {
        case FR_ANN: return 1;
        case FR_QTR:
            switch(toFreq)
            {
                case FR_ANN: return 4;
                default: return 1;
            }
        case FR_MTH: //monthly
            switch(toFreq)
            {
                case FR_ANN: return 12;
                case FR_QTR: return 3;
                default: return 1;
            }
        case FR_WK: //monthly
            switch(toFreq)
            {
                case FR_ANN: return 53;
                case FR_QTR: return 13;
                case FR_MTH: return 4;
                default: return 1;
            }
        case FR_BUS: //business
            switch(toFreq)
            {
                case FR_ANN: return maxBusDaysPerYear;;
                case FR_QTR: return maxBusDaysPerQuarter;
                case FR_MTH: return maxBusDaysPerMonth;
                case FR_WK: return 5;
                default: return 1;
            }
        case FR_DAY: //daily
            switch(toFreq)
            {
                case FR_ANN: return maxDaysPerYear;;
                case FR_QTR: return maxDaysPerQuarter;
                case FR_MTH: return maxDaysPerMonth;
                case FR_WK: return 7;
                default: return 1;
            }
        case FR_HR: //hourly
            switch(toFreq)
            {
                case FR_ANN: return 24 * maxDaysPerYear;;
                case FR_QTR: return 24 * maxDaysPerQuarter;
                case FR_MTH: return 24 * maxDaysPerMonth;
                case FR_WK: return 24 * 7;
                case FR_DAY: return 24;
                case FR_BUS: return 24;
                default: return 1;
            }
        case FR_MIN: //minutely
            switch(toFreq)
            {
                case FR_ANN: return 24 * 60 * maxDaysPerYear;;
                case FR_QTR: return 24 * 60 * maxDaysPerQuarter;
                case FR_MTH: return 24 * 60 * maxDaysPerMonth;
                case FR_WK: return 24 * 60 * 7;
                case FR_DAY: return 24 * 60;
                case FR_BUS: return 24 * 60;
                case FR_HR: return 60;
                default: return 1;
            }
        case FR_SEC: //minutely
            switch(toFreq)
            {
                case FR_ANN: return 24 * 60 * 60 * maxDaysPerYear;;
                case FR_QTR: return 24 * 60 * 60 * maxDaysPerQuarter;
                case FR_MTH: return 24 * 60 * 60 * maxDaysPerMonth;
                case FR_WK: return 24 * 60 * 60 * 7;
                case FR_DAY: return 24 * 60 * 60;
                case FR_BUS: return 24 * 60 * 60;
                case FR_HR: return 60 * 60;
                case FR_MIN: return 60;
                default: return 1;
            }
        default: return 1;
    }
}


static char cseries_convert_doc[] = "";
static PyObject *
cseries_convert(PyObject *self, PyObject *args)
{
    PyObject *arrayTest;
    PyArrayObject *array, *newArray;
    PyArrayObject *mask, *newMask;

    PyObject *returnVal = NULL;
    PyObject *start_index_retval;

    long startIndex;
    long newStart, newStartTemp;
    long newEnd, newEndTemp;
    long newLen, newHeight;
    long i, currIndex, prevIndex;
    long nd;
    npy_intp *dim, *newIdx;
    long currPerLen;
    char *position;
    int fromFreq, toFreq;
    char relation;

    PyObject *val, *valMask;

    long (*asfreq_main)(long, char) = NULL;
    long (*asfreq_endpoints)(long, char) = NULL;
    long (*asfreq_reverse)(long, char) = NULL;

    returnVal = PyDict_New();

    if (!PyArg_ParseTuple(args, "OiislO:convert(array, fromfreq, tofreq, position, startIndex, mask)", &array, &fromFreq, &toFreq, &position, &startIndex, &mask)) return NULL;

    if (toFreq == fromFreq)
    {
        PyDict_SetItemString(returnVal, "values", (PyObject*)array);
        PyDict_SetItemString(returnVal, "mask", (PyObject*)mask);

        Py_DECREF(array);
        Py_DECREF(mask);

        return returnVal;
    }

    switch(position[0])
    {
        case 'S':
            // start -> before
            relation = 'B';
            break;
        case 'E':
            // end -> after
            relation = 'A';
            break;
        default:
            return NULL;
            break;
    }

    asfreq_main = get_asfreq_func(fromFreq, toFreq, 1);
    asfreq_endpoints = get_asfreq_func(fromFreq, toFreq, 0);

    //convert start index to new frequency
    newStartTemp = asfreq_main(startIndex, 'B');
    if (newStartTemp < 1) { newStart = asfreq_endpoints(startIndex, 'A'); }
    else { newStart = newStartTemp; }

    //convert end index to new frequency
    newEndTemp = asfreq_main(startIndex+array->dimensions[0]-1, 'A');
    if (newEndTemp < 1) { newEnd = asfreq_endpoints(startIndex+array->dimensions[0]-1, 'B'); }
    else { newEnd = newEndTemp; }

    if (newStart < 1) {
        PyErr_SetString(PyExc_ValueError, "start_date outside allowable range for destination frequency");
        return NULL;
    }

    newLen = newEnd - newStart + 1;
    newHeight = get_height(fromFreq, toFreq);

    if (newHeight > 1) {

        asfreq_reverse = get_asfreq_func(toFreq, fromFreq, 0);
        currPerLen = startIndex - asfreq_reverse(newStart, 'B');

        nd = 2;
        dim = PyDimMem_NEW(nd);
        dim[0] = (npy_intp)newLen;
        dim[1] = (npy_intp)newHeight;
    } else {
        nd = 1;
        dim = PyDimMem_NEW(nd);
        dim[0] = (npy_intp)newLen;
    }

    newIdx = PyDimMem_NEW(nd);
    arrayTest = PyArray_SimpleNew(nd, dim, array->descr->type_num);
    if (arrayTest == NULL) { return NULL; }
    newArray = (PyArrayObject*)arrayTest;
    newMask  = (PyArrayObject*)PyArray_SimpleNew(nd, dim, mask->descr->type_num);

    PyDimMem_FREE(dim);

    PyArray_FILLWBYTE(newArray,0);
    PyArray_FILLWBYTE(newMask,1);

    prevIndex = newStart;

    //set values in the new array
    for (i = 0; i < array->dimensions[0]; i++) {

        val = PyArray_GETITEM(array, PyArray_GetPtr(array, &i));
        valMask = PyArray_GETITEM(mask, PyArray_GetPtr(mask, &i));

        currIndex = asfreq_main(startIndex + i, relation);
        newIdx[0] = currIndex-newStart;

        if (newHeight > 1) {

                if (currIndex != prevIndex)
                {
                    //reset period length
                    currPerLen = 0;
                    prevIndex = currIndex;
                }

                newIdx[1] = currPerLen;
                currPerLen++;
        }

        if (newIdx[0] > -1) {
            PyArray_SETITEM(newArray, PyArray_GetPtr(newArray, newIdx), val);
            PyArray_SETITEM(newMask, PyArray_GetPtr(newMask, newIdx), valMask);
        }

        Py_DECREF(val);
        Py_DECREF(valMask);
    }

    PyDimMem_FREE(newIdx);

    start_index_retval = (PyObject*)PyInt_FromLong(newStart);

    PyDict_SetItemString(returnVal, "values", (PyObject*)newArray);
    PyDict_SetItemString(returnVal, "mask", (PyObject*)newMask);
    PyDict_SetItemString(returnVal, "startindex", start_index_retval);

    Py_DECREF(newArray);
    Py_DECREF(newMask);
    Py_DECREF(start_index_retval);

    return returnVal;

}


static char cseries_asfreq_doc[] = "";
static PyObject *
cseries_asfreq(PyObject *self, PyObject *args)
{
    PyArrayObject *fromDates, *toDates;
    PyArrayIterObject *iterFrom, *iterTo;
    PyObject *fromDateObj, *toDateObj;
    char *relation;
    int fromFreq, toFreq;
    long fromDate, toDate;
    long (*asfreq_main)(long, char) = NULL;

    if (!PyArg_ParseTuple(args, "Oiis:asfreq(fromDates, fromfreq, tofreq, relation)", &fromDates, &fromFreq, &toFreq, &relation)) return NULL;

    asfreq_main = get_asfreq_func(fromFreq, toFreq, 0);

    toDates = (PyArrayObject *)PyArray_Copy(fromDates);

    iterFrom = (PyArrayIterObject *)PyArray_IterNew((PyObject *)fromDates);
    if (iterFrom == NULL) return NULL;

    iterTo = (PyArrayIterObject *)PyArray_IterNew((PyObject *)toDates);
    if (iterTo == NULL) return NULL;

    while (iterFrom->index < iterFrom->size) {

        fromDateObj = PyArray_GETITEM(fromDates, iterFrom->dataptr);
        fromDate = PyInt_AsLong(fromDateObj);
        toDate = asfreq_main(fromDate, relation[0]);
        toDateObj = PyInt_FromLong(toDate);

        PyArray_SETITEM(toDates, iterTo->dataptr, toDateObj);

        Py_DECREF(fromDateObj);
        Py_DECREF(toDateObj);

        PyArray_ITER_NEXT(iterFrom);
        PyArray_ITER_NEXT(iterTo);
    }

    Py_DECREF(iterFrom);
    Py_DECREF(iterTo);

    return (PyObject *)toDates;

}

static long dInfo_year(mxDateTimeObject *dateObj)    { return dateObj->year; }
static long dInfo_quarter(mxDateTimeObject *dateObj) { return ((dateObj->month-1)/3)+1; }
static long dInfo_month(mxDateTimeObject *dateObj)   { return dateObj->month; }
static long dInfo_day(mxDateTimeObject *dateObj)     { return dateObj->day; }
static long dInfo_day_of_year(mxDateTimeObject *dateObj)     { return dateObj->day_of_year; }
static long dInfo_day_of_week(mxDateTimeObject *dateObj)     { return dateObj->day_of_week; }
static long dInfo_week(mxDateTimeObject *dateObj)     {

    int year, week, day;
    PyObject *ISOWeekTuple = NULL;
    ISOWeekTuple = PyObject_GetAttrString((PyObject*)dateObj, "iso_week");

    if (!PyArg_ParseTuple(ISOWeekTuple,"iii;need a ISO Week 3-tuple (year,week,day)",
              &year, &week, &day)) return -9999;

    Py_DECREF(ISOWeekTuple);

    return (long)week;
}
static long dInfo_hour(mxDateTimeObject *dateObj)     { return dateObj->hour; }
static long dInfo_minute(mxDateTimeObject *dateObj)     { return dateObj->minute; }
static long dInfo_second(mxDateTimeObject *dateObj)     { return (long)dateObj->second; }

static double getAbsTime(int freq, long dailyDate, long originalDate) {

    long startOfDay, periodsPerDay;

    switch(freq)
    {
        case FR_HR:
            periodsPerDay = 24;
            break;
        case FR_MIN:
            periodsPerDay = 24*60;
            break;
        case FR_SEC:
            periodsPerDay = 24*60*60;
            break;
        default:
            return 0;
    }

    startOfDay = asfreq_DtoHIGHFREQ(dailyDate, 'B', periodsPerDay);
    return (24*60*60)*((double)(originalDate - startOfDay))/((double)periodsPerDay);
}



static char cseries_getDateInfo_doc[] = "";
static PyObject *
cseries_getDateInfo(PyObject *self, PyObject *args)
{
    int freq;
    char *info;

    PyArrayObject *array;
    PyArrayObject *newArray;
    PyArrayIterObject *iterSource, *iterResult;
    mxDateTimeObject *convDate;

    PyObject *val;
    long dateNum, dInfo;
    long absdate;
    double abstime;

    long (*toDaily)(long, char) = NULL;
    long (*getDateInfo)(mxDateTimeObject*) = NULL;

    if (!PyArg_ParseTuple(args, "Ois:getDateInfo(array, freq, info)", &array, &freq, &info)) return NULL;
    newArray = (PyArrayObject *)PyArray_Copy(array);

    iterSource = (PyArrayIterObject *)PyArray_IterNew((PyObject *)array);
    iterResult = (PyArrayIterObject *)PyArray_IterNew((PyObject *)newArray);

    toDaily = get_asfreq_func(freq, FR_DAY, 0);

    switch(*info)
    {
        case 'Y': //year
            getDateInfo = &dInfo_year;
            break;
        case 'Q': //quarter
            getDateInfo = &dInfo_quarter;
            break;
        case 'M': //month
            getDateInfo = &dInfo_month;
            break;
        case 'D': //day
            getDateInfo = &dInfo_day;
            break;
        case 'R': //day of year
            getDateInfo = &dInfo_day_of_year;
            break;
        case 'W': //day of week
            getDateInfo = &dInfo_day_of_week;
            break;
        case 'I': //week of year
            getDateInfo = &dInfo_week;
            break;
        case 'H': //hour
            getDateInfo = &dInfo_hour;
            break;
        case 'T': //minute
            getDateInfo = &dInfo_minute;
            break;
        case 'S': //second
            getDateInfo = &dInfo_second;
            break;
        default:
            return NULL;
    }

    while (iterSource->index < iterSource->size) {

        val = PyArray_GETITEM(array, iterSource->dataptr);
        dateNum = PyInt_AsLong(val);
        absdate = toDaily(dateNum, 'B');
        abstime = getAbsTime(freq, absdate, dateNum);

        convDate = (mxDateTimeObject *)mxDateTime.DateTime_FromAbsDateAndTime(absdate, abstime);
        dInfo = getDateInfo(convDate);
        Py_DECREF(convDate);

        PyArray_SETITEM(newArray, iterResult->dataptr, PyInt_FromLong(dInfo));

        PyArray_ITER_NEXT(iterSource);
        PyArray_ITER_NEXT(iterResult);
    }

    Py_DECREF(iterSource);
    Py_DECREF(iterResult);

    return (PyObject *) newArray;

}

///////////////////////////////////////////////////////////////////////

static PyMethodDef cseries_methods[] = {
    {"convert", cseries_convert, METH_VARARGS, cseries_convert_doc},
    {"asfreq", cseries_asfreq, METH_VARARGS, cseries_asfreq_doc},
    {"getDateInfo", cseries_getDateInfo, METH_VARARGS, cseries_getDateInfo_doc},
    {NULL, NULL}
};

PyMODINIT_FUNC
initcseries(void)
{
    PyObject *m, *TSER_CONSTANTS;
    m = Py_InitModule3("cseries", cseries_methods, cseries_doc);
    mxDateTime_ImportModuleAndAPI();
    import_array();

    TSER_CONSTANTS = PyDict_New();

    // Add all the frequency constants to a python dictionary
    ADD_INT_TO_DICT(TSER_CONSTANTS, "FR_ANN", FR_ANN);
    ADD_INT_TO_DICT(TSER_CONSTANTS, "FR_QTR", FR_QTR);
    ADD_INT_TO_DICT(TSER_CONSTANTS, "FR_MTH", FR_MTH);
    ADD_INT_TO_DICT(TSER_CONSTANTS, "FR_WK", FR_WK);
    ADD_INT_TO_DICT(TSER_CONSTANTS, "FR_BUS", FR_BUS);
    ADD_INT_TO_DICT(TSER_CONSTANTS, "FR_DAY", FR_DAY);
    ADD_INT_TO_DICT(TSER_CONSTANTS, "FR_HR", FR_HR);
    ADD_INT_TO_DICT(TSER_CONSTANTS, "FR_MIN", FR_MIN);
    ADD_INT_TO_DICT(TSER_CONSTANTS, "FR_SEC", FR_SEC);
    ADD_INT_TO_DICT(TSER_CONSTANTS, "FR_UND", FR_UND);

    PyModule_AddObject(m, "TSER_CONSTANTS", TSER_CONSTANTS);
}