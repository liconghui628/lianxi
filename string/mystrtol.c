
long int mystrtol(const char* nptr, char** endptr, int base)
{
	int i = 0, j = 0, k = 0,positive = 1;
	long int retvalue = 0;
	char *retptr;
	char errptr[50] = {0};
	char ptr[20] = {0};
	if(!nptr || base < 0 || base == 1 || base > 36){
		if(endptr){
			retptr = "param error !";
			while(retptr[i]){
				errptr[i] = retptr[i];
				i++;
			}
			*endptr = errptr;
		}
		return 0x7fffffff;
	}
	i = 0;
	while(nptr[i]){
		if(nptr[i] == '-')  //有 '-' 表示是个负数
			positive = 0;
		if(base == 0){//base == 0
			//16进制
			if((nptr[i] == '0' && nptr[i+1] && nptr[i+2])
					&& (nptr[i+1] == 'x' || nptr[i+1] == 'X') 
						&& ((nptr[i+2] >= '0' && nptr[i+2] <= '9') 
							|| (nptr[i+2] >= 'a' && nptr[i+2] <= 'f') 
								|| (nptr[i+1] >= 'A' && nptr[i+1] <= 'F'))){
				i += 2;
				while(nptr[i]){
					if((nptr[i] >= '0' && nptr[i] <= '9') 
							|| (nptr[i] >= 'a' && nptr[i] <= 'f') 
								|| (nptr[i] >= 'A' && nptr[i] <= 'F')){
						ptr[j++] = nptr[i];
						if(!nptr[i+1] ||
							!((nptr[i+1] >= '0' && nptr[i+1] <= '9') 
								|| (nptr[i+1] >= 'a' && nptr[i+1] <= 'f') 
									|| (nptr[i+1] >= 'A' && nptr[i+1] <= 'F'))){
							for(k = 0; k < j; k++){
								int z,n = 1;
								for(z = j-k-1;z > 0; z--)
									n *= 16;
								if(ptr[k] >= '0' && ptr[k] <= '9')
									retvalue += (ptr[k] - 48) * n;
								else if(ptr[k] >= 'A' && ptr[k] < 'F')
									retvalue += (ptr[k] - 55) * n;
								else 
									retvalue += (ptr[k] - 87) * n;
							}
							if(!positive)
								retvalue = 0 - retvalue;
							if(endptr){
								int z = 0;
								while(nptr[i+1] && z < 50){
									errptr[z++] = nptr[i+1]; 
									i++;
								}
								*endptr = errptr;
							}
							break;
						}
						i++;
					}
				}
			}
			//8进制
			else if(nptr[i] == '0' && (nptr[i+1] >= '0' && nptr[i+1] < '8')){
				i += 1;
				while(nptr[i]){
					if(nptr[i] >= '0' && nptr[i] < '8'){
						ptr[j++] = nptr[i];
						if(!nptr[i+1] || !(nptr[i+1] >= '0' && nptr[i+1] < '8')){
							for(k = 0; k < j; k++){
								int z,n = 1;
								for(z = j-k-1;z > 0; z--)
									n *= 8;
								retvalue += (ptr[k] - 48) * n;
							}
							if(!positive)
								retvalue = 0 - retvalue;
							if(endptr){
								int z = 0;
								while(nptr[i+1] && z < 50){
									errptr[z++] = nptr[i+1]; 
									i++;
								}
								*endptr = errptr;
							}
							break;
						}
						i++;
					}
				}
			}
			//10进制
			else if(nptr[i] >= '0' && nptr[i] <= '9'){
				while(nptr[i]){
					if(nptr[i] >= '0' && nptr[i] <= '9'){
						ptr[j++] = nptr[i];
						if(!nptr[i+1] || !(nptr[i+1] >= '0' && nptr[i+1] <= '9')){
							for(k = 0; k < j; k++){
								int z,n = 1;
								for(z = j-k-1;z > 0; z--)
									n *= 10;
								retvalue += (ptr[k] - 48) * n;
							}
							if(!positive)
								retvalue = 0 - retvalue;
							if(endptr){
								int z = 0;
								while(nptr[i+1] && z < 50){
									errptr[z++] = nptr[i+1]; 
									i++;
								}
								*endptr = errptr;
							}
							break;
						}
						i++;
					}
				}
			}
		}
		else if(base <= 10){ // 1-10 进制
			//8进制特殊情况，有0字符
			if(base == 8 && nptr[i] == '0' && nptr[i+1] && (nptr[i+1] >= '0' && nptr[i+1] < '8')){
				i += 1;
				while(nptr[i]){
					if(nptr[i] >= '0' && nptr[i] < '8'){
						ptr[j++] = nptr[i];
						if(!nptr[i+1] || !(nptr[i+1] >= '0' && nptr[i+1] < '8')){
							for(k = 0; k < j; k++){
								int z,n = 1;
								for(z = j-k-1;z > 0; z--)
									n *= 8;
								retvalue += (ptr[k] - 48) * n;
							}
							if(!positive)
								retvalue = 0 - retvalue;
							if(endptr){
								int z = 0;
								while(nptr[i+1] && z < 50){
									errptr[z++] = nptr[i+1]; 
									i++;
								}
								*endptr = errptr;
							}
							break;
						}
						i++;
					}
				}
				break;
			}
			//正常处理
			if(base == 8 && nptr[i] == '0' && nptr[i+1] && (nptr[i+1] >= '0' && nptr[i+1] < '8')){
				i += 1;
				while(nptr[i]){
					if(nptr[i] >= '0' && nptr[i] < '8'){
						ptr[j++] = nptr[i];
						if(!nptr[i+1] || !(nptr[i+1] >= '0' && nptr[i+1] < '8')){
							for(k = 0; k < j; k++){
								int z,n = 1;
								for(z = j-k-1;z > 0; z--)
									n *= 8;
								retvalue += (ptr[k] - 48) * n;
							}
							if(!positive)
								retvalue = 0 - retvalue;
							if(endptr){
								int z = 0;
								while(nptr[i+1] && z < 50){
									errptr[z++] = nptr[i+1]; 
									i++;
								}
								*endptr = errptr;
							}
							break;
						}
						i++;
					}
				}
				break;
			}
			//10进制
			else if(nptr[i] >= '0' && nptr[i] <= '9'){
				while(nptr[i]){
					if(nptr[i] >= '0' && nptr[i] <= '9'){
						ptr[j++] = nptr[i];
						if(!nptr[i+1] || !(nptr[i+1] >= '0' && nptr[i+1] <= '9')){
							for(k = 0; k < j; k++){
								int z,n = 1;
								for(z = j-k-1;z > 0; z--)
									n *= 10;
								retvalue += (ptr[k] - 48) * n;
							}
							if(!positive)
								retvalue = 0 - retvalue;
							if(endptr){
								int z = 0;
								while(nptr[i+1] && z < 50){
									errptr[z++] = nptr[i+1]; 
									i++;
								}
								*endptr = errptr;
							}
							break;
						}
						i++;
					}
				}
			}
		}
		else if(base <= 10){ // 1-10 进制
			//8进制特殊情况，有0字符
			if(nptr[i] >= '0' && nptr[i] < base + 48){
				ptr[j++] = nptr[i] ;
				if(!nptr[i+1] || (nptr[i+1] < '0' || nptr[i+1] >= base + 48)){
					for(k = 0; k < j; k++){
						int z,n = 1;
						for(z = j-k-1;z > 0; z--)
							n *= base;
						retvalue += (ptr[k] - 48) * n; 
					}
					if(!positive)
						retvalue = 0 - retvalue;
					//把非法字符传给endptr;
					if(endptr){
						int z = 0;
						while(nptr[i+1] && z < 50){
							errptr[z++] = nptr[i+1]; 
							i++;
						}
						*endptr = errptr;
					}
					break;
				}
			}
		}
		else{ // 11-36进制
			//16进制特殊情况，有0X字符
			if((base == 16 && nptr[i] == '0' && nptr[i+1] && nptr[i+2])
					&& (nptr[i+1] == 'x' || nptr[i+1] == 'X') 
						&& ((nptr[i+2] >= '0' && nptr[i+2] <= '9') 
							|| (nptr[i+2] >= 'a' && nptr[i+2] <= 'f') 
								|| (nptr[i+1] >= 'A' && nptr[i+1] <= 'F'))){
				i += 2;
				while(nptr[i]){
					if((nptr[i] >= '0' && nptr[i] <= '9') 
							|| (nptr[i] >= 'a' && nptr[i] <= 'f') 
								|| (nptr[i] >= 'A' && nptr[i] <= 'F')){
						ptr[j++] = nptr[i];
						if(!nptr[i+1] ||
							!((nptr[i+1] >= '0' && nptr[i+1] <= '9') 
								|| (nptr[i+1] >= 'a' && nptr[i+1] <= 'f') 
									|| (nptr[i+1] >= 'A' && nptr[i+1] <= 'F'))){
							for(k = 0; k < j; k++){
								int z,n = 1;
								for(z = j-k-1;z > 0; z--)
									n *= 16;
								if(ptr[k] >= '0' && ptr[k] <= '9')
									retvalue += (ptr[k] - 48) * n;
								else if(ptr[k] >= 'A' && ptr[k] < 'F')
									retvalue += (ptr[k] - 55) * n;
								else 
									retvalue += (ptr[k] - 87) * n;
							}
							if(!positive)
								retvalue = 0 - retvalue;
							if(endptr){
								int z = 0;
								while(nptr[i+1] && z < 50){
									errptr[z++] = nptr[i+1]; 
									i++;
								}
								*endptr = errptr;
							}
							break;
						}
						i++;
					}
				}
				break;
			}
			//没有0X字符
			if((nptr[i] >= '0' && nptr[i] <= '9') 
					|| (nptr[i] >= 'a' && nptr[i] <= 'a' + (base - 11)) 
						|| (nptr[i] >= 'A' && nptr[i] <= 'A' + (base - 11))){
				ptr[j++] = nptr[i] ;
				if(!nptr[i+1] 
						|| !((nptr[i+1] >= '0' && nptr[i+1] <= '9') 
							|| (nptr[i+1] >= 'a' && nptr[i+1] <= 'a' + (base - 11)) 
								|| (nptr[i+1] >= 'A' && nptr[i+1] <= 'A' + (base - 11)))){
					for(k = 0; k < j; k++){
						int z,n = 1;
						for(z = j-k-1;z > 0; z--)
							n *= base;
						if(ptr[k] >= '0' && ptr[k] <= '9')
							retvalue += (ptr[k] - 48) * n;
						else if(ptr[k] >= 'A' && ptr[k] < 'A' + (base - 11))
							retvalue += (ptr[k] - 55) * n;
						else 
							retvalue += (ptr[k] - 87) * n;
					}
					if(!positive)
						retvalue = 0 - retvalue;
					if(endptr){
						int z = 0;
						while(nptr[i+1] && z < 50){
							errptr[z++] = nptr[i+1]; 
							i++;
						}
						*endptr = errptr;
					}
					break;
				}
			}
		}
		i++;
	}
	if(i == 0 && retvalue == 0)
		retvalue = 0x7fffffff;

	return retvalue;
}


