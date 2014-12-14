#include <algorithm>

namespace {

	float coeffcos[] = { -1.000000000000000, 0.000000000000000, 0.707106769084930,
			0.923879504203796, 0.980785250663757, 0.995184719562531,
			0.998795449733734, 0.999698817729950, 0.999924719333649,
			0.999981164932251, 0.999995291233063, 0.999998807907104,
			0.999999701976776, 0.999999940395355, 1.000000000000000,
			1.000000000000000, };

	float coeffsin[] = { -0.000000000000000, -1.000000000000000, -0.707106769084930,
			-0.382683426141739, -0.195090323686600, -0.098017141222954,
			-0.049067676067352, -0.024541229009628, -0.012271538376808,
			-0.006135884672403, -0.003067956771702, -0.001533980132081,
			-0.000766990298871, -0.000383495178539, -0.000191747603822,
			-0.000095873801911, };

}


void fft(float *re, float *im, int m)
{
	int i=1;
	int n=2<<m;
	int nm1=n-1;
	int nd2=n>>1;
	int j=nd2;
	int k,l=1;
	int le;
	int le2;
	int jm1;
	int ip;

	float tr,ti;
	float ur,ui;
	float sr,si;

	for (;i<nm1;) {

		int count = std::min(nm1 - i, 256);
		for (; count--; i++) {
			if(i < j) {
				// rotate
				tr=re[j];
				ti=im[j];
				re[j]=re[i];
				im[j]=im[i];
				re[i]=tr;
				im[i]=ti;
			}

			k=nd2;

			while (k <= j) {
				j=j-k;
				k=k>>1;
			}

			j=j+k;
		}
	}

	for(;l<m+2;l++) {
		le=2<<(l-1);
		le2=le>>1;
		ur=1;
		ui=0;
		sr = coeffcos[l-1];
		si = coeffsin[l-1];

		for (j=1;j<le2+1; ) {

			int count = std::min(le2+1 - j, 256);
			for (; count--; j++) {
				jm1=j-1;

				for(i=jm1;i<nm1+1;i+=le) {
					ip=i+le2;
					tr=re[ip]*ur-im[ip]*ui;
					ti=re[ip]*ui+im[ip]*ur;
					re[ip]=re[i]-tr;
					im[ip]=im[i]-ti;
					re[i]=re[i]+tr;
					im[i]=im[i]+ti;
				}

				tr=ur;
				ur=tr*sr-ui*si;
				ui=tr*si+ui*sr;
			}
		}
	}
}

void inverse_fft(float *re, float *im, int m)
{
	int n=2<<m;
	float fn=(float)n;
	int i=0;
	for(;i<n;i++) im[i]=-im[i];

	fft(re,im,m);

	for(i=0;i<n;i++) {
		re[i]=re[i]/fn;
		im[i]=-im[i]/fn;
	}
}
