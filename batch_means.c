struct conf_int {
	double w;
	double sample_mean;
};


double autocor(double *data, size_t size, unsigned int lag)
{
    double sum = 0.0;
    double cosum = 0.0;
    double cosum_0 = 0.0;
    double mean;
    unsigned int i;

    for (i = 0; i < size - lag; i++) {
        cosum += data[i] * data[i+lag];
        cosum_0 += data[i] * data[i];
        sum += data[i];
    }
    for (i = size - lag; i < size; i++) {
        cosum_0 += data[i] * data[i];
        sum += data[i];
    }
    mean = sum / size;
    cosum = (cosum / (size - lag)) - (mean * mean);
    cosum_0 = (cosum_0 / size) - (mean * mean);

    return cosum / cosum_0;
}


struct conf_int confint(double *batch_mean, long k, double alpha){

	struct conf_int c_int;
	double sample_mean = 0.0, dev_std = 0.0;
    double t, w;
	unsigned int i;
	
	for(i = 0; i < k; i++) 
    	sample_mean += batch_mean[i];
    
    sample_mean = sample_mean / k;	
    
    for(i = 0; i < k; i++)
		dev_std += pow(batch_mean[i] - sample_mean, 2);

	dev_std = sqrt(dev_std/k);
	t = idfStudent(k - 1, 1 - alpha/2);
	w = (t * dev_std) / sqrt(k - 1);
    c_int.w = w;
	c_int.sample_mean = sample_mean;
    
	return c_int;  
}


int main()
{
    // get the number of replications

    // open the file

    
}
