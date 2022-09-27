.global _test_avx

.text
_test_avx:
	vpaddq ymm0, ymm1, ymm2
	xor rax, rax
	ret

