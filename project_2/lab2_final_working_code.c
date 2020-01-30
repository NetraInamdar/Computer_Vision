#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>

int main(int argc, char* argv[])
{
	FILE 		*fpt_1,*fpt_image,*fpt_template,*gt, *msf_original_image;
	unsigned char 	*ori_image;
	unsigned char 	*template, *final_msf, *MSF_copy;
	float	 	*mean_centered_template,*MSF;
	float 		msf_min,msf_max,msf_sum;
	char 		header[320],header1[320],header2[320],ch[1262];
	int 		col_arr[1262],row_arr[1262];
	int 		ROWS,COLS,BYTES,denom;		
	int 		ROWS1,COLS1,BYTES1;
	int		ROWS2,COLS2,BYTES2;
	int 		r,c,r2,c2,sum,template_sum,mean_val;
	int 		i,thresh,detected_count,not_detected_count;
	int 		e_count,fp,tp,fn,tn;
	float		TPR,FPR,tpr_array[256],fpr_array[256];
	struct timespec tp1,tp2;

	i=0;
	e_count=0;
	// Read and check original image:
	if ((fpt_image=fopen("parenthood.ppm","rb"))==NULL)
	{
		printf("Unable to open parenthood.ppm for reading.\n");
		exit(0);
	}
	fscanf(fpt_image,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);

	if(strcmp(header,"P5")!=0 || BYTES!=255)
	{
		printf("Not a greyscale 8-bit PPM image.\n");
		exit(0);
	}
	ori_image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
	header[0]=fgetc(fpt_image); //read whitespace char that separates header
	fread(ori_image,1,COLS*ROWS,fpt_image);
	fclose(fpt_image);

	// Read and check template image:
        if ((fpt_template=fopen("parenthood_e_template.ppm","rb"))==NULL)
        {
                printf("Unable to open parenthood_e_template.ppm for reading.\n");
                exit(0);
        }
        fscanf(fpt_template,"%s %d %d %d",header1,&COLS1,&ROWS1,&BYTES1);

        if(strcmp(header1,"P5")!=0 || BYTES1!=255)
        {
                printf("Not a greyscale 8-bit PPM image.\n");
                exit(0);
        }
        template=(unsigned char *)calloc(ROWS1*COLS1,sizeof(unsigned char));
        header1[0]=fgetc(fpt_template); //read whitespace char that separates header
        fread(template,1,COLS1*ROWS1,fpt_template);
        fclose(fpt_template);

	mean_centered_template=(float *)calloc(ROWS1*COLS1,sizeof(float));
	final_msf=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));

	printf("rows and cols:%d %d\n",ROWS,COLS);
	printf("rows1 and cols1:%d %d\n",ROWS1,COLS1); //rows:15 and colums:9
	
	printf("mean centered template:\n");
	msf_sum=0;
	for(r=0;r<ROWS1;r++)
	{
		for(c=0;c<COLS1;c++)
		{
			//printf("%d ",template[r*COLS1+c]);
			mean_centered_template[r*COLS1+c]=template[r*COLS1+c]-165.39;
			//printf("%f\t",mean_centered_template[r*COLS1+c]);
		}
		//printf("\n");
	}	
            
	//allocate memory for MSF:
	MSF=(float *)calloc(ROWS*COLS,sizeof(float));
	//MSF_copy=(int *)calloc(ROWS*COLS,sizeof(int));
	
	// Build MSF image, skipping the border points:
	printf("For Matched Spatial Filtering:\n");
	for (r=7;r<ROWS-7;r++)
	{
  	 for (c=4;c<COLS-4;c++)
 	 {
	    msf_sum=0;	    
	    for (r2=-7;r2<=7;r2++)
		for(c2=-4;c2<=4;c2++)
		  	msf_sum+= (ori_image[(r+r2)*COLS+(c+c2)])*(mean_centered_template[(r2+7)*COLS1+(c2+4)]);
	    MSF[r*COLS+c]=msf_sum; 
	 }
	}
	msf_min=MSF[4547];
	msf_max=MSF[4547];
	
	for(r=7;r<ROWS-7;r++)
	{
		for(c=4;c<COLS-4;c++)
		{
			if(MSF[r*COLS+c]>msf_max)
				msf_max=MSF[r*COLS+c];
			else if(MSF[r*COLS+c]<msf_min)
				msf_min=MSF[r*COLS+c];
		}
	}
	printf("msf_min:%f\t",msf_min);
	printf("msf max:%f\n",msf_max);
	
	// Re-scaling to 0-255 range of values:
	for(r=7;r<ROWS-7;r++)
        {
                for(c=4;c<COLS-4;c++)
                {
                        final_msf[r*COLS+c]=(int)(((MSF[r*COLS+c]-msf_min)/(msf_max-msf_min))*255);
                }
        }
	// write out MSF image to see result:
        fpt_1=fopen("MSF_original1.ppm","wb");
        fprintf(fpt_1,"P5 %d %d 255\n",COLS,ROWS);
        fwrite(final_msf,COLS*ROWS,1,fpt_1);
        fclose(fpt_1);
	
	gt=fopen("parenthood_gt.txt","r");
  	while(!feof(gt))
	{
		fscanf(gt,"%s %d %d",&ch[i],&col_arr[i],&row_arr[i]);
		//printf("%s %d %d\n",ch,*col_arr,*row_arr);
		i++;
	}
	fclose(gt);	

	// Threshold Loop:
	thresh=0;
	while(thresh < 256)
	{
		// Read and check original MSF image:
        	if ((msf_original_image=fopen("MSF_original1.ppm","rb"))==NULL)
        	{
                	printf("Unable to open MSF_original1.ppm for reading.\n");
                	exit(0);
        	}
        	fscanf(msf_original_image,"%s %d %d %d",header2,&COLS2,&ROWS2,&BYTES2);

        	if(strcmp(header2,"P5")!=0 || BYTES2!=255)
        	{
                	printf("Not a greyscale 8-bit PPM image.\n");
                	exit(0);
        	}
        	MSF_copy=(unsigned char *)calloc(ROWS2*COLS2,sizeof(unsigned char));
        	header2[0]=fgetc(msf_original_image); //read whitespace char that separates header
        	fread(MSF_copy,1,ROWS2*COLS2,msf_original_image);
        	fclose(msf_original_image);

		//printf("threshold:%d\n",thresh);
		detected_count=0;
		not_detected_count=0;
		fp=0,tp=0,fn=0,tn=0;

		for(r=0;r<ROWS2;r++)
		{
			for(c=0;c<COLS2;c++)
	  		{
				if(MSF_copy[r*COLS2+c]>thresh)
				{	MSF_copy[r*COLS2+c]=255; }
				else 
				{ 	MSF_copy[r*COLS2+c]=0; }
	  		}
		}

		fpt_1=fopen("MSF_binary1.ppm","wb");
        	fprintf(fpt_1,"P5 %d %d 255\n",COLS2,ROWS2);
        	fwrite(MSF_copy,COLS2*ROWS2,1,fpt_1);
        	fclose(fpt_1);	

		//for(r=0;r<100;r++)
			//for(c=0;c<100;c++)
				//printf("%d\t",MSF_copy[r*COLS2+c]);
		
		printf("after checking binary msf values\n");
		for(r=0;r<1262;r++)
		{
			sum=0;
			for(r2=-7;r2<=7;r2++)
				for(c2=-4;c2<=4;c2++)
					sum+= MSF_copy[(row_arr[r]+r2)*COLS2+ col_arr[r]+c2];
			//printf("%d\t",sum);

			if(sum>=255)
			{
				//printf("detected!\n");
				detected_count++;
				if(ch[r]=='e')
				{ tp++; }
				else
				{ fp++; }
			}	
			else
			{
				//printf("not detected!\n");
				not_detected_count++;
				if(ch[r]=='e')
				{ fn++; }
				else
				{ tn++; }
			}	
		}
		
		printf("\ndetected:%d\n",detected_count);
		printf("not detected:%d\n",not_detected_count);
		//printf("tp count:%d\t",tp);
		//printf("fp count:%d\n",fp);
		//printf("fn count:%d\n",fn);
		//printf("tn count:%d\n",tn);
		
		TPR=((float)(tp)) / ((float)(tp+fn));
		FPR=((float)(fp)) / ((float)(fp+tn));
		tpr_array[thresh]=TPR;
		fpr_array[thresh]=FPR;
		//printf("TPR:%.7f\t",TPR);
		//printf("FPR:%.7f\n",FPR);
		
		thresh++;
	}	
	
	clock_gettime(CLOCK_REALTIME,&tp2);
	printf("TPR VALUES:\n");
	for(i=255;i>=0;i--)
	{
		printf("%.2f,\t",tpr_array[i]);
	}
	printf("\nFPR VALUES:\n");
        for(i=255;i>=0;i--)
        {
                printf("%.2f,\t",fpr_array[i]);
        }
	printf("\n");	
}
