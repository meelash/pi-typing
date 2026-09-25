// Practice vocabulary. Lessons only use words whose letters have all been
// taught, so the lists deliberately include many words made from home-row
// letters. All words are chosen to be familiar and friendly for children.
#include "content.h"

namespace content {

static const char *const kWordsEn[] = {
	// home row
	"a", "as", "ad", "add", "adds", "all", "ask", "asks", "dad", "dads", "sad", "lad", "lads",
	"fall", "falls", "hall", "halls", "glad", "flag", "flags", "flash", "gas", "has", "had",
	"half", "dash", "shall", "salad", "salads", "alas", "flask", "lass", "ash", "aha",
	"glass", "slash", "alfalfa", "gala", "hash", "lag", "sag", "salsa", "fads",
	// + e i
	"see", "seed", "seeds", "feed", "feel", "heel", "deal", "seal", "leaf", "like", "lake",
	"kid", "kids", "hide", "side", "slide", "glide", "sled", "fish", "dish", "said", "idea",
	"lead", "head", "shed", "fed", "led", "he", "she", "fig", "figs", "jig", "hike", "file",
	"life", "ski", "skies", "silk", "egg", "eggs", "fell", "sell", "hill", "hills", "fill",
	"sea", "age", "safe", "sail", "sails", "jade", "shade", "lid", "aid", "hail", "field",
	"shield", "giggle", "giggles", "eagle", "geese", "else", "is", "if", "his", "her", "held",
	"shelf", "self", "desk", "ideas", "jelly", "idle", "kite", "keel", "sleek",
	// + r u t o
	"the", "to", "at", "it", "or", "our", "out", "red", "run", "sun", "fun", "hot", "dot",
	"got", "hat", "cat", "rat", "sat", "tea", "tree", "trees", "star", "stars", "stood",
	"road", "rose", "house", "horse", "tiger", "turtle", "rabbit", "jar", "art", "rest",
	"true", "tall", "ride", "hide", "tired", "sugar", "rug", "hug", "hugs", "jug", "tug",
	"shirt", "skirt", "goat", "boat", "toad", "door", "floor", "food", "good", "look",
	"took", "foot", "root", "shoe", "shoes", "short", "sort", "fort", "four", "for", "from",
	"after", "their", "there", "other", "otter", "toast", "juice", "fruit", "sister", "street",
	"sister", "doll", "dolls", "hold", "fold", "old", "gold", "told", "rule", "ruler",
	"story", "stories", "garden", "father", "letter", "letters", "star", "start", "tool",
	"girl", "girls", "first", "thirst", "flower", "tooth", "outside", "doodle", "turtles",
	// + c n w m
	"can", "cake", "car", "cars", "coat", "corn", "cow", "cows", "cone", "cold", "cool",
	"clock", "cloud", "clouds", "come", "cookie", "cookies", "circle", "catch", "chair",
	"chalk", "cheese", "chicken", "children", "class", "clean", "and", "in", "on", "no",
	"not", "now", "new", "nest", "nice", "nine", "noon", "nose", "net", "sing", "song",
	"ring", "king", "long", "ten", "hen", "pen", "rain", "train", "snow", "snail", "moon",
	"man", "me", "my", "mom", "milk", "mice", "mint", "mitten", "music", "melon", "lemon",
	"water", "we", "was", "win", "wind", "window", "wing", "wings", "winter", "wish", "wolf",
	"word", "words", "work", "world", "worm", "swim", "swing", "sweet", "slow", "show",
	"snack", "smile", "small", "sock", "socks", "come", "home", "time", "name", "same",
	"game", "games", "animal", "animals", "morning", "mountain", "lion", "ocean", "cinema",
	// + y p v b
	"yes", "you", "your", "yellow", "yard", "yarn", "yummy", "happy", "funny", "sunny",
	"windy", "rainy", "puppy", "pony", "play", "plays", "park", "pet", "pets", "pig", "pink",
	"pizza", "plant", "plate", "pear", "pie", "pop", "purple", "paper", "pencil", "pool",
	"prize", "party", "picnic", "pirate", "planet", "spoon", "sleep", "jump", "up", "map",
	"very", "van", "visit", "violin", "voice", "vase", "five", "give", "love", "have",
	"brave", "river", "silver", "seven", "eleven", "big", "bag", "bat", "bed", "bee",
	"bees", "bell", "bike", "bird", "birds", "blue", "boat", "book", "books", "box", "boy",
	"bread", "bus", "butter", "button", "baby", "ball", "banana", "bear", "beach", "bubble",
	"bubbles", "robot", "rocket", "table", "rabbit", "tub", "web", "grape", "grapes", "lamp",
	"try", "toy", "toys", "day", "days", "boy", "sky", "fly", "my", "by", "type",
	"typing", "keyboard", "keys", "key", "fingers", "finger", "hands",
	// + q z x
	"quiz", "queen", "quick", "quiet", "quilt", "quack", "zoo", "zero", "zip", "zebra",
	"zebras", "zigzag", "lazy", "puzzle", "pizza", "fizz", "buzz", "jazz", "size", "prize",
	"fox", "box", "six", "mix", "fix", "wax", "taxi", "next", "extra", "excited", "exit",
	"oxen", "boxes", "foxes", "sixty", "maze", "freeze", "breeze", "squirrel", "squash",
};

static const char *const kWordsAr[] = {
	// ب ت ي ن
	"بيت", "بنت", "تين", "بين", "نبت", "بني", "بنتي", "بيتي", "يبني", "تبني",
	// + س م
	"يمين", "نسيم", "مبني", "يسمي", "تمت", "بسمة",
	// + ش ك
	"شمس", "كتب", "مكتب", "كنت", "يمشي", "تمشي", "كيس", "مسك", "سمك", "كبش", "يكتب",
	"تكتب", "مكتبي", "سكين", "يكنس", "مسكين", "شمسي", "كيسي",
	// + ل ا
	"سلام", "كتاب", "باب", "ماما", "بابا", "كلب", "لبن", "اسم", "مال", "نام", "نامت",
	"ليل", "بنات", "نبات", "ملابس", "كلام", "سلم", "مكان", "بستان", "لسان", "شمال", "كتابي",
	"نمل", "بلبل", "شباك", "ناس", "نيل", "كان", "كانت", "ليس", "لك", "لي", "ما", "من",
	"لا", "كلا", "الباب", "البيت", "الشمس", "الكتاب", "الكلب", "ابن", "اسمي", "سلمى",
	"مالك", "كسلان", "بالبيت", "ابتسم", "ابتسامة", "كلاب", "كتابك", "ملك",
	// + ط
	"بط", "بطاطس", "سلطان", "طالب", "بسيط", "نشيط", "طين", "بساط", "مشط", "مطاط", "طبيب",
	"طيب", "طماطم", "طلاب", "مطبات", "طابت",
	// + ث ه
	"هنا", "هي", "هم", "هل", "بيته", "كتابه", "هيا", "مثل", "ثمن", "ثياب", "ثلاث", "اثنين",
	"اثنان", "ثابت", "مثلث", "بهما", "ثلاثين", "هاتف",
	// + ق ع
	"قلم", "علم", "عين", "عسل", "لعب", "يلعب", "تلعب", "ملعب", "قط", "عنب", "قلب", "معلم",
	"قميص", "نعم", "عمي", "قليل", "عالم", "طعام", "عشب", "قلبي", "عيني", "لعبت", "سعيد",
	// + ف غ
	"فيل", "في", "فم", "غيم", "فلفل", "فستان", "طفل", "فقط", "غني", "فن", "فنان", "سيف",
	"كيف", "لطيف", "غنم", "غسل", "يغسل", "فعل", "تفاح", "غسيل", "قفل", "فلافل", "فيلم",
	// + ص خ
	"صف", "خط", "نخل", "مطبخ", "شخص", "قصص", "خال", "خالي", "خمس", "قفص", "خفيف", "صعب",
	"خيط", "خشب", "صيف", "مقص", "صمت", "خاتم", "صابون", "صغيرة",
	// + ض ح
	"حليب", "حصان", "ضحك", "يضحك", "تمساح", "صباح", "حب", "حلم", "بيض", "حمام", "ضيف",
	"مفتاح", "ملح", "مصباح", "فلاح", "حقل", "حمص", "صحن", "صحيح", "حيتان", "مضحك",
	// + ج د
	"جمل", "دجاج", "جديد", "دب", "يد", "جد", "سعيد", "عيد", "جميل", "جبل", "مسجد", "بعيد",
	"جيد", "جامع", "صديق", "جناح", "نجم", "شجاع", "جيب", "ديك", "سجاد", "بلد", "بلاد",
	"حديد", "جدا", "محمد", "عدد", "دقيق", "صديقي", "يدي", "جديدة",
	// + ر و
	"ولد", "ورد", "نور", "قمر", "بحر", "طير", "يوم", "نوم", "لون", "حوت", "ثور", "فرس",
	"حمار", "طاووس", "ليمون", "بالون", "صور", "شجر", "مطر", "نهر", "سرير", "كبير", "صغير",
	"قرد", "نمر", "ورق", "قلوب", "حلو", "بيوت", "تمر", "فرح", "شكرا", "مرحبا", "حروف",
	"كرسي", "درس", "دروس", "مدرس", "قطار", "عصفور", "برتقال", "خيار", "رمان", "بصل",
	"ثوم", "فول", "شاي", "كوب", "رسم", "يرسم", "عمر", "شهر", "وجه", "رجل", "دولاب",
	"صندوق", "فرن", "حجر", "مرح", "قارب", "طريق", "جسر", "عربي", "روبوت", "صاروخ", "كوكب",
	"نجوم", "خروف", "ولد", "بحار", "كتبوا", "لعبوا", "سرور",
	// + ة ى
	"قطة", "بطة", "كرة", "وردة", "مدرسة", "سيارة", "شجرة", "نملة", "سمكة", "بقرة", "طاولة",
	"غابة", "لعبة", "قصة", "حديقة", "مكتبة", "نجمة", "هدية", "مدينة", "على", "مستشفى",
	"موسيقى", "ليلى", "حلوى", "متى", "حتى", "مشى", "بكى", "دقيقة", "ساعة", "شمعة",
	"ملعقة", "خيمة", "جدة", "صورة", "حقيبة", "فراشة", "حكاية", "دجاجة", "سفينة", "نخلة",
	"قلعة", "بحيرة", "لغة", "قهوة", "شوكة", "العربية", "مكنسة", "سلة", "خريطة", "رحلة",
	// + ز ء
	"موز", "خبز", "جزر", "زيت", "ماء", "سماء", "زرافة", "زهرة", "كنز", "ماعز", "زر",
	"غزال", "فضاء", "مساء", "غداء", "عشاء", "شتاء", "دواء", "هواء", "زرقاء", "خضراء",
	"بيضاء", "صفراء", "حمراء", "سوداء", "زيتون", "جزيرة", "بيتزا", "زجاج", "لوز", "مزرعة",
	"زميل", "شيء", "جزء", "هدوء", "ضوء",
	// + ؤ ئ
	"شاطئ", "دافئ", "هادئ", "مؤدب", "سؤال", "رئيس", "بئر", "مئة", "لؤلؤ", "طائرة",
	"جائزة", "قائمة", "رائع", "سائق", "عائلة", "نائم", "فائز", "بؤبؤ",
	// + ظ ذ
	"ذئب", "هذا", "هذه", "ذهب", "نظيف", "نظارة", "ظل", "ظرف", "حذاء", "لذيذ", "تلميذ",
	"ذكي", "عظيم", "ذيل", "ذراع", "ظهر", "انتظر", "استيقظ", "منظر", "حفظ", "ذرة", "لماذا",
	"ماذا", "حظ",
	// + أ إ آ
	"أب", "أم", "أنا", "أنت", "أخ", "أخت", "أسد", "أرنب", "أزرق", "أحمر", "أخضر", "أصفر",
	"أبيض", "أسود", "إلى", "إذا", "إنسان", "آلة", "مرآة", "أكل", "أمام", "أين", "أصدقاء",
	"أطفال", "أرز", "إصبع", "أصابع", "أذن", "أنف", "أسنان", "أشجار", "أزهار", "ألوان",
	"أرقام", "أمل", "إبريق", "آخر", "الآن", "أهلا", "أيضا", "سأل", "فأر", "رأس", "كأس",
};

// Sentences start with a capital letter and use only . , ' ? punctuation.
static const char *const kSentencesEn[] = {
	"The cat sat on the mat.",
	"I like to read books.",
	"We can play in the park.",
	"The sun is hot today.",
	"My dog likes to run fast.",
	"She has a red kite.",
	"Look at the big ship.",
	"Birds sing in the morning.",
	"Can you jump high?",
	"Where is my blue hat?",
	"The fish swims in the lake.",
	"I see six green frogs.",
	"Mom made a cake for me.",
	"Dad and I fixed the bike.",
	"The moon is bright tonight.",
	"We planted seeds in the garden.",
	"The quick brown fox jumps over the lazy dog.",
	"Rain falls from the gray clouds.",
	"A little bee flew by.",
	"Is it time for lunch?",
	"Please pass the milk.",
	"Thank you for the gift.",
	"The zebra has black and white stripes.",
	"I brush my teeth every night.",
	"Let's go to the zoo.",
	"It's a sunny day.",
	"Don't forget your coat.",
	"We're going to the beach.",
	"What is your name?",
	"How old are you?",
	"The train goes very fast.",
	"My friend has a pet rabbit.",
	"The kitten is asleep.",
	"We share our toys.",
	"Be kind to others.",
	"Yes, I can help you.",
	"First, wash your hands.",
	"Apples, pears, and grapes are fruit.",
	"I can type with ten fingers.",
	"Keep your eyes on the screen.",
	"Slow and steady wins the race.",
	"The owl hoots at night.",
	"Frogs jump, fish swim, and birds fly.",
	"Who wants to play a game?",
	"I'm happy to see you.",
};

static const char *const kSentencesAr[] = {
	"القطة تلعب بالكرة.",
	"الشمس ساطعة اليوم.",
	"هذا كتاب جميل.",
	"السماء زرقاء.",
	"العصفور على الشجرة.",
	"الفيل حيوان كبير.",
	"يسبح السمك في البحر.",
	"القمر جميل في الليل.",
	"البطة تسبح في البحيرة.",
	"في الشتاء ينزل المطر.",
	"هذه وردة جميلة.",
	"معلمنا لطيف.",
	"بيتنا كبير وجميل.",
	"نلعب معا في الحديقة.",
	"الولد يرسم شجرة.",
	"البنت تقرأ قصة.",
	"ذهب الولد إلى المدرسة.",
	"أحب أمي وأبي.",
	"أكلت تفاحة حمراء.",
	"في الحديقة أزهار كثيرة.",
	"أين قلمي؟",
	"ما اسمك؟",
	"كم عمرك؟",
	"أنا أحب القراءة.",
	"شكرا على الهدية.",
	"صباح الخير يا جدي.",
	"أغسل يدي قبل الطعام.",
	"أكتب بأصابعي العشرة.",
	"الدقة أولا، ثم السرعة.",
	"هل تحب الموز؟",
	"نعم، أحب الموز كثيرا.",
	"الأرنب يأكل الجزر.",
	"أمي تطبخ طعاما لذيذا.",
	"للزرافة رقبة طويلة.",
	"سافرنا بالطائرة إلى البحر.",
	"النظافة من الإيمان.",
	"انظر إلى الشاشة، لا إلى يديك.",
	"الصديق وقت الضيق.",
	"من جد وجد، ومن زرع حصد.",
	"العلم نور.",
};

const char *const *WordsFor(Lang lang, int *count)
{
	if (lang == LangAr) {
		*count = ARRAY_LEN(kWordsAr);
		return kWordsAr;
	}
	*count = ARRAY_LEN(kWordsEn);
	return kWordsEn;
}

const char *const *SentencesFor(Lang lang, int *count)
{
	if (lang == LangAr) {
		*count = ARRAY_LEN(kSentencesAr);
		return kSentencesAr;
	}
	*count = ARRAY_LEN(kSentencesEn);
	return kSentencesEn;
}

} // namespace content
