namespace Scheme.RT {
	public enum Sys : int {
		open=0,
		unlink=1,
		close=2,
		read=3,
		write=4,
		get_resource_usage=5,
		dump_heap=6,
		exit=7,
		mtime=8,
		access=9,
		rename=10,
		pollinput=11,
		getenv=12,
		gc=13,
		flonum_log=14,
		flonum_exp=15,
		flonum_sin=16,
		flonum_cos=17,
		flonum_tan=18,
		flonum_asin=19,
		flonum_acos=20,
		flonum_atan=21,
		flonum_atan2=22,
		flonum_sqrt=23,
		stats_dump_on=24,
		stats_dump_off=25,
		iflush=26,
		gcctl=27,
		block_signals=28,
		flonum_sinh=29,
		flonum_cosh=30,
		system=31,
		c_ffi_apply=32,
		c_ffi_dlopen=33,
		c_ffi_dlsym=34,
		make_nonrelocatable=35,
		object_to_address=36,
		ffi_getaddr=37,
		sro=38,
		sys_feature=39,
		peek_bytes=40,
		poke_bytes=41,
		segment_code_address=42,
		stats_dump_stdout=43,
		chdir=44,
		cwd=45,
		sysglobal = 91
	};
}