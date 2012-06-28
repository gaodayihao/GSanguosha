echo "// ****************auto-generated resource mappings**********************" > images.skin;
echo "// equips" >> images.skin;
ls -1 equips/*.png | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{print "\"photoEquip-" $1 "\": \"image/equips/icon/" $0 "\",";
	 print "\"dashboardEquip-" $1 "\": \"image/equips/" $0 "\","}
' >> images.skin;
echo "// kingdoms" >> images.skin;
ls -1 kingdom/frame/*.png | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{print "\"kingdomColorMask-" $1 "\": \"image/kingdom/frame/" $0 "\",";
	 print "\"kingdomIcon-" $1 "\": \"image/kingdom/icon/" $0 "\","}
' >> images.skin;
echo "// cards" >> images.skin;
echo "\"handCardMainPhoto-unknown\": \"image/system/card-back.png\"," >> images.skin;
ls -1 card/*.jpg | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	($1 != "unknown"){print "\"handCardMainPhoto-" $1 "\": \"image/card/" $0 "\","}
' >> images.skin;
ls -1 generals/card/*.jpg | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{
		if ($1 == "xushu" || $1 == "fazheng" || $1 == "lingtong")
		{
			print "\"handCardMainPhoto-nos" $1 "\": \"image/generals/card/" $0 "\","
		}
		print "\"handCardMainPhoto-" $1 "\": \"image/generals/card/" $0 "\","
	}
' >> images.skin;
ls -1 system/black/*.png | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{print "\"handCardNumber-black-" $1 "\": \"image/system/black/" $0 "\","}
' >> images.skin;
ls -1 system/red/*.png | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{print "\"handCardNumber-red-" $1 "\": \"image/system/red/" $0 "\","}
' >> images.skin;
ls -1 system/cardsuit/*.png | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{print "\"handCardSuit-" $1 "\": \"image/system/cardsuit/" $0 "\","}
' >> images.skin;
ls -1 icon/*.png | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{print "\"judgeCardIcon-" $1 "\": \"image/icon/" $0 "\","}
' >> images.skin; 
echo "// generals" >> images.skin;
ls -1 generals/tiny/*.png | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{
		if ($1 == "xushu" || $1 == "fazheng" || $1 == "lingtong")
		{
			print "\"handCardAvatar-nos" $1 "-0\": \"image/generals/tiny/" $0 "\","
		}
		print "\"handCardAvatar-" $1 "-0\": \"image/generals/tiny/" $0 "\","
	}
' >> images.skin;

echo "// generals" >> images.skin;
ls -1 generals/tiny/*.png | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{
		if ($1 == "xushu" || $1 == "fazheng" || $1 == "lingtong")
		{
			print "\"playerGeneralIcon-nos" $1 "-0\": \"image/generals/tiny/" $0 "\","
		}
		print "\"playerGeneralIcon-" $1 "-0\": \"image/generals/tiny/" $0 "\","
	}
' >> images.skin;
ls -1 generals/small/*.png | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{
		if ($1 == "xushu" || $1 == "fazheng" || $1 == "lingtong")
		{
			print "\"playerGeneralIcon-nos" $1 "-1\": \"image/generals/small/" $0 "\","
		}
		print "\"playerGeneralIcon-" $1 "-1\": [\"image/generals/card/" $1 ".jpg\",  [40, 26, 148, 89]], "
	}
' >> images.skin;
ls -1 generals/big/*.png | xargs -n1 basename | awk  '
	BEGIN {FS = "."}
	{
		if ($1 == "xushu" || $1 == "fazheng" || $1 == "lingtong")
		{
			print "\"playerGeneralIcon-nos" $1 "-2\": \"image/generals/big/" $0 "\","
		}
		print "\"playerGeneralIcon-" $1 "-2\": \"image/generals/big/" $0 "\","
	}
' >> images.skin;

