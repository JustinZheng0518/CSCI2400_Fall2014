#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rtdsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  // Took declarations and definitions outside of the loop, so the loop won't have to keep redeclaring it, this will prevent unrequired excess declarations.
  struct cs1300bmp *input = new struct cs1300bmp;
  struct cs1300bmp *output = new struct cs1300bmp;
  int inNum;
  string inputFilename;
  string outputFilename;
  int ok;
  double sample;



  for (inNum = 2; inNum < argc; inNum++)
  {
    inputFilename = argv[inNum];
    
    ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if(ok)
    {
      sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  // Moved these outside
  int size, div, i, j, value;
  
  if ( ! input.bad() )
  {
    size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    
    input >> div;
    filter -> setDivisor(div);
     
    for (i=0; i < size; i++)
    {
      for (j=0; j < size; j++) 
      {
	    input >> value;
	    filter -> set(i,j,value);
      }
    }
    return filter;
  }
}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  // Moved all variables within the loops, these are the constant ones
  int inHeight = (input -> height) - 1;
  int inWidth = (input -> width) - 1;
  int sizeFilter = filter ->getSize();
  int divisor = filter -> getDivisor();
  
  output -> width = input -> width;
  output -> height = input -> height;
  
  // Moved all variables within the loops out here, also declared a few parallel vars to prevent bottlenecking
  int row, col, plane, i, value, value2, value3, newRow1, newRow3, newCol1, newCol3;
  // New short integer array for the filter types.
  int filterArray[3][3];
  // Defines a parallel region, which is code that will be executed by multiple threads in parallel
  #pragma omp parallel for
  for (i = 0; i < 3; i++)
  {
	  // I originally used nested for loops for filling this array, but found a more optimum method, shown below. Similar to Parallelled vars, except not multiplication.
      filterArray[i][0] = filter->get(i,0);
      filterArray[i][1] = filter->get(i,1);
      filterArray[i][2] = filter->get(i,2);
  }

  // hline
  if (filterArray[0][1] == -2)
  {
    #pragma omp parallel for
    for(plane = 0; plane < 3; plane++)
    {
      for(row = 1; row < inHeight; row++)
      {
        newRow1 = row - 1;
        newRow3 = row + 1;

        for(col = 1; col < inWidth; col++)
        {
          newCol1 = col - 1;
          newCol3 = col + 1;

		  value=0;
          value2=0;
          value3=0;

          value += -(input->color[plane][newRow1][newCol1]);
          value2 += -(input->color[plane][newRow1][col] << 1);
          value3 += -(input->color[plane][newRow1][newCol3]);
          value += input->color[plane][newRow3][newCol1];
          value2 += input->color[plane][newRow3][col] << 1;
          value3 += input->color[plane][newRow3][newCol3];

          value += value2+value3;

          if (value > 255)
			value = 255;
          if (value < 0)
			value = 0;

          output->color[plane][row][col] = value;
        }
      }
    }
  }

  // Gauss
  else if (filterArray[1][1] == 8){
    #pragma omp parallel for
    for(plane = 0; plane < 3; plane++)
    {
      for(row = 1; row < inHeight; row++)
      {
        newRow1 = row - 1;
        newRow3 = row + 1;

        for(col = 1; col < inWidth; col++)
        {
          newCol1 = col - 1;
          newCol3 = col + 1;

		  value=0;
          value2=0;
          value3=0;

          value2 += input->color[plane][newRow1][col] << 2;
          value += input->color[plane][row][newCol1] << 2;
          value2 += input->color[plane][row][col] << 3;
          value3 += input->color[plane][row][newCol3] << 2;
          value2 += input->color[plane][newRow3][col] << 2;

          // Dividing the value by 24 for Gauss
          value = ((value+value2+value3) >> 3)/3;

          if (value > 255)
			value = 255;
          if (value < 0)
			value = 0;

          output->color[plane][row][col] = value;
        }
      }
    }
  }

  // Emboss
  else if (filterArray[1][2] == -1)
  {
    #pragma omp parallel for
    for(plane = 0; plane < 3; plane++)
    {
      for(row = 1; row < inHeight; row++)
      {
        newRow1 = row - 1;
        newRow3 = row + 1;
        
        for(col = 1; col < inWidth; col++)
        {
          newCol1 = col - 1;
          newCol3 = col + 1;

		  value=0;
          value2=0;
          value3=0;

          value += input->color[plane][newRow1][newCol1];
          value2 += input->color[plane][newRow1][col];
          value3 += -(input->color[plane][newRow3][col]);
          value += input->color[plane][row][newCol1];
          value2 += input->color[plane][row][col];
          value3 += -(input->color[plane][newRow3][col]);
          value += input->color[plane][newRow3][newCol1];
          value2 += -(input->color[plane][newRow3][col]);
          value3 += -(input->color[plane][newRow3][newCol3]);

          value += value2+value3;

          if (value > 255)
			value = 255;
          if (value < 0)
			value = 0;

          output->color[plane][row][col] = value;
        }
      }
    }
  }
  
  // Other Filter Cases
  else
  {
    #pragma omp parallel for
    for(plane = 0; plane < 3; plane++)
    {
      for(row = 1; row < inHeight; row++)
      {
        newRow1 = row - 1;
        newRow3 = row + 1;

        for(col = 1; col < inWidth; col++)
        {
          newCol1 = col - 1;
          newCol3 = col + 1;
		  
		  value=0;
          value2=0;
          value3=0;
		  
          value += input->color[plane][newRow1][newCol1];
          value2 += input->color[plane][newRow1][col];
          value3 += input->color[plane][newRow1][newCol3];
          value += input->color[plane][row][newCol1];
          value2 += input->color[plane][row][col];
          value3 += input->color[plane][row][newCol3];
          value += input->color[plane][newRow3][newCol1];
          value2 += input->color[plane][newRow3][col];
          value3 += input->color[plane][newRow3][newCol3];

          value = (value+value2+value3)/9;

          if (value > 255)
			value = 255;
          if (value < 0)
			value = 0;
			
          output->color[plane][row][col] = value;
        }
      }
    }
  }
  
/*
 * ORIGINAL MODIFICATIONS, This brought the CPE down to near 500 on my Computer, but I decided to split this code into multiple conditions, to help reduce loop usage.
  for(plane = 0; plane < 3; plane++)
  {
    for(row = 1; row < inHeight; row++)
    {
      for(col = 1; col < inWidth; col++)
      {

	    value = 0;
	    value2 = 0;
	    value3 = 0;
	    
	    for (i = 0; i < sizeFilter; i++)
	    {
	      for (j = 0; j < sizeFilter-3; j += 3)
	      {
	        value += input -> color[plane][row + i - 1][col + j - 1] * filter -> get(i, j);
	        value2 += input -> color[plane][row + i - 1][col + j+1 - 1] * filter -> get(i, j+1);
	        value3 += input -> color[plane][row + i - 1][col + j+2 - 1] * filter -> get(i, j+2);
	      }
	      for (; j < sizeFilter; j++)
	      {
		    value += input -> color[plane][row + i - 1][col + j - 1] * filter -> get(i, j);
		  }
	    }
	    
	    value += value2 + value3;
	    value /= divisor;
	    
	    if ( value  < 0 ) { value = 0; }
	    if ( value  > 255 ) { value = 255; }

	    output -> color[plane][row][col] = value;
      }
    }
  }
*/

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}
