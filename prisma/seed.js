const {PrismaClient} = require('@prisma/client');
const prisma         = new PrismaClient();

async function main() {
    await prisma.task.createMany({
        data: [
            {description: '散步 15 分鐘', mood: ['放鬆'], suggestedTime: 15},
            {description: '閱讀 20 分鐘', mood: ['靜心'], suggestedTime: 20},
            {description: '整理房間', mood: ['積極'], suggestedTime: 30},
            {description: '泡杯咖啡', mood: ['放鬆'], suggestedTime: 10},
            {description: '寫下三件感恩小事', mood: ['靜心'], suggestedTime: 10},
        ],
    });
}

main()
    .then(() => {
        console.log('✅ Seed inserted');
    })
    .catch(console.error)
    .finally(() => prisma.$disconnect());
